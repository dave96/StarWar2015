#include "Player.hh"
#include <stack>
#include <map>
#include <utility>
#include <queue>
#include <vector>

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
    
    struct nodo {
		Pos pos;
		int turno;
		int misiles;
	};
    
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
	bool suitable(const Pos& c) { return within_window(c, round()) and (cell(c).type == EMPTY or cell(c).type == MISSILE or cell(c).type == MISSILE_BONUS or cell(c).type == POINT_BONUS); }
	
	nodo make_nodo(const Pos& pos, const int& turno, const int& misiles) {
		nodo ret;
		ret.pos = pos;
		ret.turno = turno;
		ret.misiles = misiles;
		return ret;
	}
	
	// Comprobar si hay alguien delante para matar
		  
	  bool check_shoot(const Starship& s, const Pos& p) {
			Cell ahead = cell(p+DEFAULT);
			if (ahead.type == STARSHIP) {
				if (player_of(ahead.sid) != me()) {
					shoot(s.sid);
					return true;
				}
			} else if (cell(p+FAST).type == STARSHIP and player_of(cell(p+FAST).sid) != me()) {
				shoot(s.sid);
				return true;
			}
			return false;	
	  }
	  
	  // Quiero saber donde está el peligro más cercano (en la ventana). Si no hay, devuelvo p.
	  
	  bool danger (const Pos& p) {
		  for(int i = 2; i <= 4; ++i) {
			  Pos check = {p.real(), p.imag() - i};
			  if (cell(check).type == MISSILE) return true;
		  }
		  return false;
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
	  
	  bool check_colision(const Pos& p, const Dir& d) {
		if (d == DEFAULT or d == SLOW_UP or d == SLOW_DOWN) return suitable(p+d);
		if (d == UP) return suitable(p+SLOW_UP) and suitable(p+DEFAULT) and suitable(p+UP);
		if (d == DOWN) return suitable(p+SLOW_DOWN) and suitable(p+DEFAULT) and suitable(p+DOWN);
		if (d == FAST_UP) return check_colision(p, UP) and suitable(p+FAST) and suitable(p+FAST_UP);
		if (d == FAST_DOWN) return check_colision(p, DOWN) and suitable(p+FAST) and suitable(p+FAST_DOWN);
		if (d == FAST) return suitable(p+DEFAULT) and suitable(p+FAST);
		return false;
	  }
	  
	  void print_pos(const Pos& p) {
		  cerr << "{" << p.real() << "," << p.imag() << "}" << endl;
	  }
	  
	  bool free_obj(const Pos& p, const int& s_indx) {
		  for(int i = 0; i < number_starships_per_player(); ++i) if(i != s_indx and objective[i].first == p) return false;
		  return true;
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
	   
	   bool enqueue(map <Pos, nodo, pos_comp>& m, const Pos& p, const Dir& d, queue<Pos>& q, const nodo& n, const CType& obj, const int& s_indx) {
		   map<Pos, nodo, pos_comp>::iterator it;
		   if(within_window(p+d, n.turno+1)) {
				   it = m.find(p+d);
				   if (it == m.end()) { // Posición no visitada con aterioridad.
					   if (cell(p+d).type == obj and free_obj(p+d, s_indx)) {
							// Encontrado
							// cerr << "Encontrado" << endl;
							m[p+d] = make_nodo(p, n.turno+1, n.misiles);
							stack_movements(m, p+d, s_indx);
							active_objective[s_indx] = true;
							return true;
						} else if (cell(p+d).type != ASTEROID)  {
							m[p+d] = make_nodo(p, n.turno+1, n.misiles);
							q.push(p+d);
							return false;
						
						} else if (d == DEFAULT and n.misiles > 0) {
							m[p+d] = make_nodo(p, n.turno+1, n.misiles-1);
							q.push(p+d);
							return false;
						}
					}
				}
			return false;
	   }
	   
	   void stack_movements(map<Pos, nodo, pos_comp>& m, Pos p, const int& s_indx) {
		   objective[s_indx].second = stack<Dir>();
		   objective[s_indx].first = p;
		   map<Pos, pair<Pos, int>, pos_comp>::iterator it;
		   while(m[p].pos != p) {
			   objective[s_indx].second.push(p - m[p].pos);
			   p = m[p].pos;
		   }
	   }
	   
	   void objective_search(const Pos& p, const CType& obj) {
		   int s_indx = cell(p).sid - begin(me());
		   int misiles_inicial = starship(cell(p).sid).nb_miss;
		   active_objective[s_indx] = false;
		   // Definimos estructuras auxiliares.
		   map<Pos, nodo, pos_comp> positions;
		   queue<Pos> q;
		   positions[p] = make_nodo(p, round(), misiles_inicial);
		   q.push(p);
		   bool ok = true;
		   while (not q.empty()) {
			   Pos x = q.front(); q.pop();
			   nodo ant = positions[x];
			   if (within_window(x, round())) {
				   if (enqueue(positions, x, DEFAULT, q, ant, obj, s_indx)) return;
				   if(suitable(x+DEFAULT) and enqueue(positions, x, FAST, q, ant, obj, s_indx)) return;
				   // Parte complicada, definir con que está conectado cada celda.
					if(enqueue(positions, x, SLOW_UP, q, ant, obj, s_indx)) return;
					if(enqueue(positions, x, SLOW_DOWN, q, ant, obj, s_indx)) return;
					if (suitable (x+SLOW_UP) and suitable(x+DEFAULT)) {
						if(enqueue(positions, x, UP, q, ant, obj, s_indx)) return;
						if(suitable(x+UP) and suitable(x+FAST) and enqueue(positions, x, FAST_UP, q, ant, obj, s_indx)) return;
					}
					if (suitable (x+DEFAULT) and suitable(x+SLOW_DOWN)) {
						if (enqueue(positions, x, DOWN, q, ant, obj, s_indx)) return;
						if (suitable(x+DOWN) and suitable(x+FAST) and enqueue(positions, x, FAST_DOWN, q, ant, obj, s_indx)) return;
					}
				}
		   }
	   }
	   
	   Dir get_safe_move(const Pos& p) {
		   if (suitable(p+SLOW_UP)) {
			   if (not danger(p+SLOW_UP)) return SLOW_UP;
			   if (suitable(p+UP) and suitable(p+DEFAULT)) {
				   if (not danger(p+UP)) return UP;
				   if (suitable(p+FAST) and suitable(p+FAST_UP) and not danger(p+FAST_UP)) return FAST_UP;
			   }
		   }
		   if (suitable(p+SLOW_DOWN)) {
			   if (not danger(p+SLOW_DOWN)) return SLOW_DOWN;
			   if (suitable(p+DOWN) and suitable(p+DEFAULT)) {
				   if (not danger(p+DOWN)) return DOWN;
				   if (suitable(p+FAST) and suitable(p+FAST_DOWN) and not danger(p+FAST_DOWN)) return FAST_DOWN;
			   }
		   }
		   if (suitable(p+DEFAULT) and suitable(p+FAST) and not danger(p+FAST)) return FAST;
		   return DEFAULT;
			   
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
		  bool jugado = false;
		  if (s.alive) {
			Pos p = s.pos;		  
			if (s.nb_miss > 0 and (not danger(p)) and check_shoot(s, p)) {
				// Vale, perfecto pero si teníamos un objetivo nos hemos cargado la cosa.
				active_objective[s_indx] = false;
				jugado = true;
			}
			
			if((not(active_objective[s_indx]) or objective[s_indx].second.empty()) and not jugado) {
				// cerr << "Voy a buscar" << endl;
				if (s.nb_miss == 0) objective_search(p, MISSILE_BONUS);
				else {
					objective_search(p, POINT_BONUS);
					if (not active_objective[s_indx]) objective_search(p, MISSILE_BONUS);
				}
				// cerr << "Busqueda hecha, resultado "<< active_objective[s_indx] << endl;
			}
			
			if (active_objective[s_indx] and not jugado) {
				if (cell(objective[s_indx].first).type != EMPTY and not danger(p+objective[s_indx].second.top())) {
					if (check_colision(p, objective[s_indx].second.top())) {
						move(sid, objective[s_indx].second.top());
						objective[s_indx].second.pop();
						// cerr << "Muevo nave sin colision" << endl;
						jugado = true;
					} else if (objective[s_indx].second.top() == DEFAULT and s.nb_miss > 0) {
						shoot(sid);
						objective[s_indx].second.pop();
						// cerr << "Muevo nave disparando" << endl;
						jugado = true;
					}
				}
				if (not jugado) {
					// cerr << "Objetivo ya no válido, busco alternativa" << endl;
					if (s.nb_miss == 0) objective_search(p, MISSILE_BONUS);
					else {
						objective_search(p, POINT_BONUS);
						if (not active_objective[s_indx]) objective_search(p, MISSILE_BONUS);
					}
					if (active_objective[s_indx] and not danger(p+objective[s_indx].second.top())) {
						if(objective[s_indx].second.top() == DEFAULT and not suitable(p+DEFAULT)) {
							// cerr << "Disparo en segunda busqueda" << endl;
							shoot(sid);
							jugado = true;
						} else {
							print_pos(objective[s_indx].second.top());
							// // cerr << cell(p+objective[s_indx].second.top()).type << endl;
							move(sid, objective[s_indx].second.top());
							objective[s_indx].second.pop();
							// cerr << "Muevo en segunda busqueda" << endl;
							jugado = true;
						}
					}
				}
			}
			if (not jugado) {
				// cerr << "Sin objetivos" << endl;
				if (within_window(p, round()+1) and not danger(p)) move(sid, SLOW);
				else move(sid, get_safe_move(p));
			}
			
		} else {
			active_objective[s_indx] = false;
			} // End if (s.alive)
		
	  } // End for(starship)
  } // End virtual void play()

};


/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);
