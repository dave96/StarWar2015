#include "Player.hh"
#include <stack>
#include <map>
#include <utility>
#include <queue>
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
     
    bool active_objective = false;
	Pos objective;
	stack<Dir> pendent_moves;
	
	bool updown(Dir d)   { return d == SLOW_UP   or d == UP   or d == FAST_UP or d == SLOW_DOWN or d == DOWN or d == FAST_DOWN;   }
	
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
	   
	   bool enqueue(map <Pos, pair<Pos, int>, pos_comp>& m, const Pos& p, const Dir& d, queue<Pos>& q, const int& l, const CType& obj) {
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
							stack_movements(m, p+d);
							objective = p+d;
							active_objective = true;
							return true;
						}
					}
					return false;
				}
	   }
	   
	   void stack_movements(const map<Pos, pair<Pos, int>, pos_comp>& m, const Pos& p) {
		   
	   }
	   
	   void objective_search(const Pos& p, const CType& obj, const int& limit) {
		   active_objective = false;
		   // Definimos estructuras auxiliares.
		   map<Pos, pair<Pos, int>, pos_comp> positions;
		   queue<Pos> q;
		   q.push(p);
		   while (not q.empty()) {
			   Pos x = q.front(); q.pop();
			   int xlimit = positions[x].second;
			   assert (xlimit <= limit);
			   // Parte complicada, definir con que está conectado cada celda.
			   if (xlimit < limit) {
				   if(enqueue(positions, p, SLOW_UP, q, xlimit+1, obj)) return;
				   if(enqueue(positions, p, SLOW_DOWN, q, xlimit+1, obj)) return;
				}
				if (cell(p+SLOW_UP).type == EMPTY or cell(p+SLOW_UP).type == MISSILE) if(enqueue(positions, p, UP, q, xlimit, obj)) return;
				if (cell(p+SLOW_DOWN).type == EMPTY or cell(p+SLOW_DOWN).type == MISSILE) if(enqueue(positions, p, DOWN, q, xlimit, obj)) return;
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
		  
		}

		// Por cada nave.
		for (Starship_Id sid = begin(me()); sid != end(me()); ++sid) {

		  Starship s = starship(sid);

		  if (s.alive) { 

			Pos p = s.pos;
			
			// Me la voy a pegar? Esto me debería decir si me va a dar un misil.
			
			Pos danger = nearest_missile(p);
			bool too_close = (p - danger == DEFAULT);

			// Si hay alguien delante, dispara
			
			if (s.nb_miss > 0 and (not too_close) and check_shoot(s, p)) {
				// Vale, perfecto pero si teníamos un objetivo nos hemos cargado la cosa.
				active_objective = false;
				return;
			}
			
		} // End if (s.alive)
		
	  } // End for(starship)
  } // End virtual void play()

};


/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);
