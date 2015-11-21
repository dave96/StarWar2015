#include "Player.hh"
#include <stack>
#include <map>
#include <utility>
#include <queue>
#include <vector>
#include <cassert>

using namespace std;


/**
 * Write the name of your player and save this file
 * as AI<name>.cc
 */
#define PLAYER_NAME BobaFett

struct PLAYER_NAME : public Player {


    /**
     * Factory: returns a new instance of this class.
     * Do not modify this function.
     */
    static Player* factory () {
        return new PLAYER_NAME;
    }
    
    const Dir BACK = {0, -1};
    
    struct pos_comp {
		bool operator() (const Pos& p1, const Pos& p2) {
			if (p1.real() < p2.real()) return true;
			if (p1.real() == p2.real()) return p1.imag() < p2.imag();
			return false;
		}
	};


    /**
     * Attributes for your player can be defined here.
     */
     
    vector<bool> active_objective;
    vector<pair<Pos, stack<Dir>>> objective;
	
	bool updown(Dir d)   { return d == SLOW_UP   or d == UP   or d == FAST_UP or d == SLOW_DOWN or d == DOWN or d == FAST_DOWN;   }
	bool suitable(const Pos& c) { return within_window(c, round()) and (cell(c).type == EMPTY or cell(c).type == MISSILE); }
	
	// Comprobar si hay alguien delante para matar
		  
	  bool check_shoot(const Starship& s, const Pos& p) {
			Cell ahead = cell(p+DEFAULT);
			if (ahead.type == STARSHIP) {
				if (player_of(ahead.sid) != me()) {
					shoot(s.sid);
					return true;
				}
			} else if ((ahead = cell(p+FAST)).type == STARSHIP and player_of(ahead.sid) != me()) {
				shoot(s.sid);
				return true;
			}
			return false;	
	  }
	  
	  // Quiero saber donde está el peligro más cercano (en la ventana). Si no hay, devuelvo p.
	  
	  Pos nearest_missile (const Pos& p) {
		  Pos iter = p+BACK;
		  while(within_window(iter, round())) {
			  Cell icell = cell(iter);
			  if (icell.type == ASTEROID) return p;
			  else if (icell.type == MISSILE or (icell.type == STARSHIP and player_of(icell.sid) != me())) return iter;
			  iter += BACK;
		}
		return p;
	  }
	  
	  int get_limit(Pos p) {
		  int limit = 0;
		  p -= BACK;
		  while (within_window(p, round())) {
			  limit++;
			  p -= BACK;
		  }
		  return limit;
	  }
	  
	  
	  /**
	   * Esto es una implementación del algoritmo BFS, aplicado a las posiciones.
	   * Vamos a suponer que cada posición del mapa es un nodo de un grafo dirigido,
	   * y es dirigido porque de un nodo NO podemos ir atrás. La idea es usarlo para
	   * encontrar el bonus/misil más cercano DENTRO de la ventana.
	   * 
	   * También vamos a tener en cuenta lo siguiente: si estamos muy cerca de la parte izquierda de la ventana, 
	   * no podremos ir rectos hacia abajo demasiadas veces, por decirlo así, porque llegaríamos al límite de la ventana.
	   * 
	   * Al final del algoritmo calcularemos los movimientos a hacer y los colocaremos en un stack, así podremos ir 
	   * leyendo los movimientos a realizar de uno en uno según pasen los turnos.
	   * 
	   * La implementación interna va a usar maps o unordered_maps (ya veremos). 
	   */
	   
	   bool enqueue(map <Pos, pair<Pos, int>, pos_comp>& m, const Pos& p, const Dir& d, queue<Pos>& q, const int& l, const CType& obj, const int& s_indx) {
		   assert(dir_ok(d)); // DEBUG
		   map<Pos, pair<Pos, int>, pos_comp>::iterator it;
		   if(within_window(p+d, round())) {
				   it = m.find(p+d);
				   if (it == m.end()) { // Posición no visitada con aterioridad.
						if (cell(p+d).type == EMPTY or cell(p+d).type == MISSILE)  {
							m[p+d] = make_pair(p, l);
							q.push(p+d);
							return false;
						} else if (cell(p+d).type == obj) {
							// Encontrado
							m[p+d] = make_pair(p, l);
							stack_movements(m, p+d, s_indx);
							active_objective[s_indx] = true;
							return true;
						}
					}
					return false;
				}
	   }
	   
	   void stack_movements(map<Pos, pair<Pos, int>, pos_comp>& m, Pos p, const int& s_indx) {
		   objective[s_indx].second = stack<Dir>();
		   objective[s_indx].first = p;
		   map<Pos, pair<Pos, int>, pos_comp>::iterator it;
		   while(m[p].first != p) {
			   assert(dir_ok(p - m[p].first));
			   objective[s_indx].second.push(p - m[p].first);
			   p = m[p].first;
		   }
	   }
	   
	   void objective_search(const Pos& p, const CType& obj, const int& limit) {
		   int s_indx = cell(p).sid - begin(me());
		   active_objective[s_indx] = false;
		   // Definimos estructuras auxiliares.
		   map<Pos, pair<Pos, int>, pos_comp> positions;
		   queue<Pos> q;
		   positions[p] = make_pair(p, 0);
		   q.push(p);
		   while (not q.empty()) {
			   Pos x = q.front(); q.pop();
			   int xlimit = positions[x].second;
			   assert (xlimit <= limit);
			   if (enqueue(positions, p, DEFAULT, q, xlimit, obj, s_indx)) return;
			   if (enqueue(positions, p, FAST, q, xlimit, obj, s_indx)) return;
			   // Parte complicada, definir con que está conectado cada celda.
			   if (xlimit < limit) {
				   if(enqueue(positions, p, SLOW_UP, q, xlimit+1, obj, s_indx)) return;
				   if(enqueue(positions, p, SLOW_DOWN, q, xlimit+1, obj, s_indx)) return;
				}
				if (suitable(p+SLOW_UP) and enqueue(positions, p, UP, q, xlimit, obj, s_indx)) return;
				if (suitable(p+SLOW_DOWN) and enqueue(positions, p, DOWN, q, xlimit, obj, s_indx)) return;
				if (suitable(p+DEFAULT) and suitable(p+UP) and enqueue(positions, p, FAST_UP, q, xlimit-1, obj, s_indx)) return;
				if (suitable(p+DEFAULT) and suitable(p+DOWN) and enqueue(positions, p, FAST_DOWN, q, xlimit-1, obj, s_indx)) return;
		   }
	   }


    /**
     * Play method.
     *
     * This method will be invoked once per each round.
     * You have to read the board here to place your actions
     * for this round.
     *
     */
	  virtual void play () {

		if (round() == 0) {
		  // Inicializar tipos de datos.
		  active_objective = vector<bool> (number_starships_per_player(), false);
		  objective = vector<pair<Pos, stack<Dir>>> (number_starships_per_player());
		}

		// Por cada nave.
		for (Starship_Id sid = begin(me()); sid != end(me()); ++sid) {

		  Starship s = starship(sid);
		  int s_indx = sid - begin(me());
		  if (s.alive) { 

			Pos p = s.pos;
			
			// Me la voy a pegar? Esto me debería decir si me va a dar un misil.
			
			Pos danger = nearest_missile(p);
			bool too_close = (p - danger == DEFAULT);
			
			if (too_close) {
				if (not(active_objective[s_indx])) {
					if (suitable(UP)) move(sid, UP);
					else move(sid,DOWN);
				} else {
					if (!updown(objective[s_indx].second.top())) {
						if (suitable(UP)) move(sid, UP);
						else move(sid,DOWN);
					} else {
						move(sid, objective[s_indx].second.top());
						objective[s_indx].second.pop();
					}
				}
			}

			// Si hay alguien delante, dispara
			
			if (s.nb_miss > 0 and check_shoot(s, p)) {
				// Vale, perfecto pero si teníamos un objetivo nos hemos cargado la cosa.
				shoot(sid);
				active_objective[s_indx] = false;
				return;
			}
			
			if (active_objective[s_indx] and suitable(objective[s_indx].second.top())) {
				move(sid, objective[s_indx].second.top());
				objective[s_indx].second.pop();
				return;
			}
			
			int limit = get_limit(p);
			
			if (s.nb_miss == 0) objective_search(p, MISSILE_BONUS, limit);
			else objective_search(p, POINT_BONUS, limit);
			
			if (not active_objective[s_indx]) {
				if (limit > 0) move(sid, SLOW);
				else move(sid, DEFAULT);
				return;
			}
			
			move(sid, objective[s_indx].second.top());
			objective[s_indx].second.pop();
			return;
			
		} // End if (s.alive)
		
	  } // End for(starship)
  } // End virtual void play()

};


/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);
