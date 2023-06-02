#ifndef POKE327_H
# define POKE327_H

# include <stdlib.h>
# include <assert.h>
#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>
#include <cstdio>

using namespace std;

# include "heap.h"

class character_t;

#define malloc(size) ({          \
  void *_tmp;                    \
  assert((_tmp = malloc(size))); \
  _tmp;                          \
})

/* Returns true if random float in [0,1] is less than *
 * numerator/denominator.  Uses only integer math.    */
# define rand_under(numerator, denominator) \
  (rand() < ((RAND_MAX / denominator) * numerator))

/* Returns random integer in [min, max]. */
# define rand_range(min, max) ((rand() % (((max) + 1) - (min))) + (min))

# define UNUSED(f) ((void) f)

typedef enum dim {
  dim_x,
  dim_y,
  num_dims
} dim_t;

typedef int16_t pair_t[num_dims];

#define MAP_X              80
#define MAP_Y              21
#define MIN_TREES          10
#define MIN_BOULDERS       10
#define TREE_PROB          95
#define BOULDER_PROB       95
#define WORLD_SIZE         401
#define MIN_TRAINERS       7   
#define ADD_TRAINER_PROB   50

#define mappair(pair) (m->map[pair[dim_y]][pair[dim_x]])
#define mapxy(x, y) (m->map[y][x])
#define heightpair(pair) (m->height[pair[dim_y]][pair[dim_x]])
#define heightxy(x, y) (m->height[y][x])

typedef enum __attribute__ ((__packed__)) terrain_type {
  ter_boulder,
  ter_tree,
  ter_path,
  ter_mart,
  ter_center,
  ter_grass,
  ter_clearing,
  ter_mountain,
  ter_forest,
  ter_exit,
  num_terrain_types
} terrain_type_t;

class map_t {
public:
  terrain_type_t map[MAP_Y][MAP_X];
  uint8_t height[MAP_Y][MAP_X];
  character_t *cmap[MAP_Y][MAP_X];
  heap_t turn;
  int32_t num_trainers;
  int8_t n, s, e, w;
};

class npc_t;
class pc_t;
/* Here instead of character.h to abvoid including character.h */
class character_t {
public:
  npc_t *npc;
  pc_t *pc;
  pair_t pos;
  char symbol;
  int next_turn;
};

class world_t {
public:
  map_t *world[WORLD_SIZE][WORLD_SIZE];
  pair_t cur_idx;
  map_t *cur_map;
  /* Please distance maps in world, not map, since *
   * we only need one pair at any given time.      */
  int hiker_dist[MAP_Y][MAP_X];
  int rival_dist[MAP_Y][MAP_X];
  character_t pc;
  int quit;
};

class pokemon {
public:
    int id, species_id, height, weight, base_exp, order, is_default;
    string name;
    
    pokemon(int id, string name, int species_id, int height, int weight, int base_exp, int order, int is_default) {
        this->id = id;
        this->name = name;
        this->species_id = species_id;
        this->height = height;
        this->weight = weight;
        this->base_exp = base_exp;
        this->order = order;
        this->is_default = is_default;
    }
    
    void print_pokemon() {
        cout << "ID Number: " << id << ", Name: " << name << ", Species ID Number: " << species_id << ", Height: " << height << ", Weight: " << weight
        << ", Base EXP: " << base_exp << ", Order: " << order << ", Default?: " << is_default << endl;
    }
};

class pokemon_moves {
public:
    int pokemon_id, version_group_id, move_id, pokemon_move_method_id, level, order;
    
    pokemon_moves(int pokemon_id, int version_group_id, int move_id, int pokemon_move_method_id, int level, int order) {
        this->pokemon_id = pokemon_id;
        this->version_group_id = version_group_id;
        this->move_id = move_id;
        this->pokemon_move_method_id = pokemon_move_method_id;
        this->level = level;
        this->order = order;
    }
    
    void print_pokemon_moves() {
        if(pokemon_id != -1) {
            cout << "Pokemon ID: " << pokemon_id;
        }
        if(version_group_id != -1) {
            cout << ", Version Group ID: " << version_group_id;
        }
        if(move_id != -1) {
            cout << ", Move ID: " << move_id;
        }
        if(pokemon_move_method_id != -1) {
            cout << ", Pokemon Move Method ID: " << pokemon_move_method_id;
        }
        if(level != -1) {
            cout << ", Level: " << level;
        }
        if(order != -1) {
            cout << ", Order: " << order << endl;
        }
        cout << endl;
    }
};

class moves {
public:
    int id, generation_id, type_id, power, pp, accuracy, priority, target_id, damage_class_id, effect_id, effect_chance, contest_type_id, contest_effect_id, super_contest_effect_id;
    string name;
    
    moves(int id, string name, int generation_id, int type_id, int power, int pp, int accuracy, int priority, int target_id, int damage_class_id, int effect_id, int effect_chance,
          int contest_type_id, int contest_effect_id, int super_contest_effect_id) {
        this->id = id;
        this->name = name;
        this->generation_id = generation_id;
        this->type_id = type_id;
        this->power = power;
        this->pp = pp;
        this->accuracy = accuracy;
        this->priority = priority;
        this->target_id = target_id;
        this->damage_class_id = damage_class_id;
        this->effect_id = effect_id;
        this->effect_chance = effect_chance;
        this->contest_type_id = contest_type_id;
        this->contest_effect_id = contest_effect_id;
        this->super_contest_effect_id = super_contest_effect_id;
    }
    
    void print_moves() {
        if(id != -1) {
            cout << "ID: " << id << ", Name: " << name;
        }
        if(generation_id != -1) {
            cout << ", Generation ID: " << generation_id;
        }
        if(type_id != -1) {
            cout << ", Type ID: " << type_id;
        }
        if(power != -1) {
            cout << ", Power: " << power;
        }
        if(pp != -1) {
            cout << ", PP: " << pp;
        }
        if(accuracy != -1) {
            cout << ", Accuracy: " << accuracy;
        }
        if(priority != -1) {
            cout << ", Priority: " << priority;
        }
        if(target_id != -1) {
            cout << ", Target ID: " << target_id;
        }
        if(damage_class_id != -1) {
            cout << ", Damage Class ID: " << damage_class_id;
        }
        if(effect_id != -1) {
            cout << ", Effect ID: " << effect_id;
        }
        if(effect_chance != -1) {
            cout << ", Effect Chance: " << effect_chance;
        }
        if(contest_type_id != -1) {
            cout << ", Contest Type ID: " << contest_type_id;
        }
        if(contest_effect_id != -1) {
            cout << ", Contest Effect ID: " << contest_effect_id;
        }
        if(super_contest_effect_id != -1) {
            cout << ", Super Contest Effect ID: " << super_contest_effect_id << endl;
        }
        cout << endl;
    }
};

class pokemon_stats {
public:
    int pokemon_id, stat_id, base_stat, effort;
    
    pokemon_stats(int pokemon_id, int stat_id, int base_stat, int effort) {
        this->pokemon_id = pokemon_id;
        this->stat_id = stat_id;
        this->base_stat = base_stat;
        this->effort = effort;
    }
};

/* Even unallocated, a WORLD_SIZE x WORLD_SIZE array of pointers is a very *
 * large thing to put on the stack.  To avoid that, world is a global.     */
extern world_t world;

extern pair_t all_dirs[8];

extern vector<pokemon> pokes;
extern vector<pokemon_moves> pokemon_moves_vec;
extern vector<pokemon_moves> pokemon_moves_20_vec;
extern vector<moves> moves_vec;
extern vector<pokemon_stats> poke_stats_vec;

#define rand_dir(dir) {     \
  int _i = rand() & 0x7;    \
  dir[0] = all_dirs[_i][0]; \
  dir[1] = all_dirs[_i][1]; \
}

class path_t {
public:
  heap_node_t *hn;
  uint8_t pos[2];
  uint8_t from[2];
  int32_t cost;
};

int new_map(int teleport);

#endif