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
#define PLAYER_NAME JohnCena

struct PLAYER_NAME : public Player {


    /**
     * Factory: returns a new instance of this class.
     * Do not modify this function.
     */
    static Player* factory () {
        return new PLAYER_NAME;
    }
    
    const Dir BACK = {0, -1};
    const int AGRO = 2;
    const int AVANCE = 7;
    
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
    vector<Pos> next_obj;
	
	inline bool updown(Dir d)   { return d == SLOW_UP   or d == UP   or d == FAST_UP or d == SLOW_DOWN or d == DOWN or d == FAST_DOWN;   }
	inline bool suitable(const Pos& c) { return within_window(c, round()) and (cell(c).type == EMPTY or cell(c).type == MISSILE or cell(c).type == MISSILE_BONUS or cell(c).type == POINT_BONUS); }
	
	bool not_occupied(const Pos& p) {
		   int max = next_obj.size();
		   for (int i = 0; i < max; ++i) if (next_obj[i] == p) return false;
		   return true;
	}
	
	nodo make_nodo(const Pos& pos, const int& turno, const int& misiles) {
		nodo ret;
		ret.pos = pos;
		ret.turno = turno;
		ret.misiles = misiles;
		return ret;
	}
	
	// Comprobar si hay alguien delante para matar
		  
	  bool check_shoot(const Starship& s, Pos p) {
		for(int i = 0; i < AGRO; ++i) {
			p += DEFAULT;
			if (cell(p).type == STARSHIP and player_of(cell(p).sid) != me()) {
				shoot(s.sid);
				return true;
			} else if (cell(p).type != EMPTY) return false;
		}
		return false;
	  }
	  
	  // Quiero saber donde estÃ¡ el peligro mÃ¡s cercano (en la ventana). Si no hay, devuelvo p.
	  
	  bool danger (const Pos& p) {
		  for(int i = 1; i <= 4; ++i) {
			  Pos check = {p.real(), p.imag() - i};
			  // cerr << "Celda: " << endl;
			  // print_pos(check);
			  if (within_universe(check)) {
				if (cell(check).type == ASTEROID or cell(check).type == POINT_BONUS or cell(check).type == MISSILE_BONUS) return false;
				if (cell(check).type == MISSILE) return true;
			  }
			  // cerr << "Fin celda" << endl;
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
		if (d == UP) return suitable(p+SLOW_UP) and suitable(p+DEFAULT) and suitable(p+UP) and not_occupied(p+SLOW_UP) and not_occupied(p+DEFAULT);
		if (d == DOWN) return suitable(p+SLOW_DOWN) and suitable(p+DEFAULT) and suitable(p+DOWN) and not_occupied(p+SLOW_DOWN) and not_occupied(p+DEFAULT);
		if (d == FAST_UP) return check_colision(p, UP) and suitable(p+FAST) and suitable(p+FAST_UP) and not_occupied(p+UP) and not_occupied(p+FAST);
		if (d == FAST_DOWN) return check_colision(p, DOWN) and suitable(p+FAST) and suitable(p+FAST_DOWN) and not_occupied(p+DOWN) and not_occupied(p+FAST);
		if (d == FAST) return suitable(p+DEFAULT) and suitable(p+FAST) and not_occupied(p+DEFAULT);
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
	   * Esto es una implementaciÃƒÂ³n del algoritmo BFS, aplicado a las posiciones.
	   * Vamos a suponer que cada posiciÃƒÂ³n del mapa es un nodo de un grafo dirigido,
	   * y es dirigido porque de un nodo NO podemos ir atrÃƒÂ¡s. La idea es usarlo para
	   * encontrar el bonus/misil mÃƒÂ¡s cercano DENTRO de la ventana.
	   * 
	   * TambiÃƒÂ©n vamos a tener en cuenta lo siguiente: si estamos muy cerca de la parte izquierda de la ventana, 
	   * no podremos ir rectos hacia abajo demasiadas veces, por decirlo asÃƒÂ­, porque llegarÃƒÂ­amos al lÃƒÂ­mite de la ventana.
	   * 
	   * Al final del algoritmo calcularemos los movimientos a hacer y los colocaremos en un stack, asÃƒÂ­ podremos ir 
	   * leyendo los movimientos a realizar de uno en uno segÃƒÂºn pasen los turnos.
	   * 
	   * La implementaciÃƒÂ³n interna va a usar maps o unordered_maps (ya veremos). 
	   */
	   
	   bool enqueue(map <Pos, nodo, pos_comp>& m, const Pos& p, const Dir& d, queue<Pos>& q, const nodo& n, const CType& obj, const int& s_indx) {
		   map<Pos, nodo, pos_comp>::iterator it;
		   if(within_window(p+d, n.turno+1)) {
				   it = m.find(p+d);
				   if (it == m.end()) { // PosiciÃ³n no visitada con aterioridad.
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
			   if (within_window(x, round()) or within_window(x, round()+AVANCE)) {
				   if (enqueue(positions, x, DEFAULT, q, ant, obj, s_indx)) return;
				   if(suitable(x+DEFAULT) and enqueue(positions, x, FAST, q, ant, obj, s_indx)) return;
				   // Parte complicada, definir con que estÃƒÂ¡ conectado cada celda.
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
		next_obj = vector<Pos>();
		// Por cada nave.
		for (Starship_Id sid = begin(me()); sid != end(me()); ++sid) {
		  Starship s = starship(sid);
		  int s_indx = sid - begin(me());
		  // cerr << "Comienzo de nave " << s_indx << endl;
		  bool jugado = false;
		  if (s.alive) {
			Pos p = s.pos;
			// cerr << "Compruebo si hay disparo posible" << endl;
			if (s.nb_miss > 0 and (not danger(p)) and check_shoot(s, p)) {
				// Vale, perfecto pero si tenÃƒÂ­amos un objetivo nos hemos cargado la cosa.
				active_objective[s_indx] = false;
				jugado = true;
				next_obj.push_back(p+DEFAULT);
			}
			// cerr << "Comprobado. Vamos a ver si tengo objetivo" << endl;
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
				// cerr << "Tengo objetivo" << endl;
				if (cell(objective[s_indx].first).type != EMPTY and not danger(p+objective[s_indx].second.top())) {
					// cerr << "Objetivo sin peligro y no vacÃ­Â­o" << endl;
					if (check_colision(p, objective[s_indx].second.top()) and not_occupied(p+objective[s_indx].second.top())) {
						move(sid, objective[s_indx].second.top());
						next_obj.push_back(p+objective[s_indx].second.top());
						objective[s_indx].second.pop();
						// cerr << "Muevo nave sin colision" << endl;
						jugado = true;
					} else if (objective[s_indx].second.top() == DEFAULT and s.nb_miss > 0) {
						shoot(sid);
						next_obj.push_back(p+DEFAULT);
						objective[s_indx].second.pop();
						// cerr << "Muevo nave disparando" << endl;
						jugado = true;
					}
				}
				// cerr << "Objetivo con peligro o vacÃ­o" << endl;
				if (not jugado) {
					// cerr << "Objetivo ya no vÃ¡lido, busco alternativa" << endl;
					if (s.nb_miss == 0) objective_search(p, MISSILE_BONUS);
					else {
						objective_search(p, POINT_BONUS);
						if (not active_objective[s_indx]) objective_search(p, MISSILE_BONUS);
					}
					if (active_objective[s_indx] and not danger(p+objective[s_indx].second.top())) {
						if(objective[s_indx].second.top() == DEFAULT and (not suitable(p+DEFAULT)) and not_occupied(p+DEFAULT)) {
							// cerr << "Disparo en segunda busqueda" << endl;
							shoot(sid);
							next_obj.push_back(p+DEFAULT);
							jugado = true;
						} else if (not_occupied(p+objective[s_indx].second.top()) and check_colision(p, objective[s_indx].second.top())) {
							// print_pos(objective[s_indx].second.top());
							// cerr << cell(p+objective[s_indx].second.top()).type << endl;
							move(sid, objective[s_indx].second.top());
							next_obj.push_back(p+objective[s_indx].second.top());
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
				else {
					Dir d = get_safe_move(p);
					next_obj.push_back(p+d);
					move(sid, d);
				} 
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
