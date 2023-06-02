#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>
#include <ncurses.h>
#include <stdbool.h>
#include <curses.h>

#include "heap.h"

#define malloc(size) ({          \
  void *_tmp;                    \
  assert((_tmp = malloc(size))); \
  _tmp;                          \
})

typedef struct path {
  heap_node_t *hn;
  uint8_t pos[2];
  uint8_t from[2];
  int32_t cost;
} path_t;

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
  num_terrain_types
} terrain_type_t;

typedef enum __attribute__ ((__packed__)) movement_type {
  move_hiker,
  move_rival,
  move_pace,
  move_wander,
  move_sentry,
  move_walk,
  move_pc,
  num_movement_types
} movement_type_t;

typedef enum __attribute__ ((__packed__)) character_type {
  char_pc,
  char_hiker,
  char_rival,
  char_other,
  num_character_types
} character_type_t;

static int32_t move_cost[num_character_types][num_terrain_types] = {
  { INT_MAX, INT_MAX, 10, 10,      10,      20, 10, INT_MAX, INT_MAX },
  { INT_MAX, INT_MAX, 10, INT_MAX, INT_MAX, 15, 10, 15,      15      },
  { INT_MAX, INT_MAX, 10, INT_MAX, INT_MAX, 20, 10, INT_MAX, INT_MAX },
  { INT_MAX, INT_MAX, 10, INT_MAX, INT_MAX, 20, 10, INT_MAX, INT_MAX },
};

typedef struct pc {
} pc_t;

typedef struct npc {
  character_type_t ctype;
  movement_type_t mtype;
  pair_t dir;
} npc_t;

typedef struct character {
  npc_t *npc;
  pc_t *pc;
  pair_t pos;
  char symbol;
  int next_turn;
  bool defeated;
} character_t;

typedef struct map {
  terrain_type_t map[MAP_Y][MAP_X];
  uint8_t height[MAP_Y][MAP_X];
  character_t *cmap[MAP_Y][MAP_X];
  heap_t turn;
  int8_t n, s, e, w;
} map_t;

typedef struct queue_node {
  int x, y;
  struct queue_node *next;
} queue_node_t;

typedef struct world {
  map_t *world[WORLD_SIZE][WORLD_SIZE];
  pair_t cur_idx;
  map_t *cur_map;
  int hiker_dist[MAP_Y][MAP_X];
  int rival_dist[MAP_Y][MAP_X];
  character_t pc;
} world_t;

world_t world;

static pair_t all_dirs[8] = {
  { -1, -1 },
  { -1,  0 },
  { -1,  1 },
  {  0, -1 },
  {  0,  1 },
  {  1, -1 },
  {  1,  0 },
  {  1,  1 },
};

#define rand_dir(dir) {     \
  int _i = rand() & 0x7;    \
  dir[0] = all_dirs[_i][0]; \
  dir[1] = all_dirs[_i][1]; \
}


static void move_hiker_func(character_t *c, pair_t dest){
  int min;
  int base;
  int i;
  
  base = rand() & 0x7;

  dest[dim_x] = c->pos[dim_x];
  dest[dim_y] = c->pos[dim_y];
  min = INT_MAX;
  
  for (i = base; i < 8 + base; i++) {
    if ((world.hiker_dist[c->pos[dim_y] + all_dirs[i & 0x7][dim_y]]
                         [c->pos[dim_x] + all_dirs[i & 0x7][dim_x]] <=
         min) &&
        !world.cur_map->cmap[c->pos[dim_y] + all_dirs[i & 0x7][dim_y]]
                            [c->pos[dim_x] + all_dirs[i & 0x7][dim_x]]) {
      dest[dim_x] = c->pos[dim_x] + all_dirs[i & 0x7][dim_x];
      dest[dim_y] = c->pos[dim_y] + all_dirs[i & 0x7][dim_y];
      min = world.hiker_dist[dest[dim_y]][dest[dim_x]];
    }
  }
}

static void move_rival_func(character_t *c, pair_t dest){
  int min;
  int base;
  int i;
  
  base = rand() & 0x7;

  dest[dim_x] = c->pos[dim_x];
  dest[dim_y] = c->pos[dim_y];
  min = INT_MAX;
  
  for (i = base; i < 8 + base; i++) {
    if ((world.rival_dist[c->pos[dim_y] + all_dirs[i & 0x7][dim_y]]
                         [c->pos[dim_x] + all_dirs[i & 0x7][dim_x]] <
         min) &&
        !world.cur_map->cmap[c->pos[dim_y] + all_dirs[i & 0x7][dim_y]]
                            [c->pos[dim_x] + all_dirs[i & 0x7][dim_x]]) {
      dest[dim_x] = c->pos[dim_x] + all_dirs[i & 0x7][dim_x];
      dest[dim_y] = c->pos[dim_y] + all_dirs[i & 0x7][dim_y];
      min = world.rival_dist[dest[dim_y]][dest[dim_x]];
    }
  }
}

static void move_pacer_func(character_t *c, pair_t dest){
  dest[dim_x] = c->pos[dim_x];
  dest[dim_y] = c->pos[dim_y];

  if ((world.cur_map->map[c->pos[dim_y] + c->npc->dir[dim_y]]
                         [c->pos[dim_x] + c->npc->dir[dim_x]] !=
       world.cur_map->map[c->pos[dim_y]][c->pos[dim_x]]) ||
      world.cur_map->cmap[c->pos[dim_y] + c->npc->dir[dim_y]]
                         [c->pos[dim_x] + c->npc->dir[dim_x]]) {
    c->npc->dir[dim_x] *= -1;
    c->npc->dir[dim_y] *= -1;
  }

  if ((world.cur_map->map[c->pos[dim_y] + c->npc->dir[dim_y]]
                         [c->pos[dim_x] + c->npc->dir[dim_x]] ==
       world.cur_map->map[c->pos[dim_y]][c->pos[dim_x]]) &&
      !world.cur_map->cmap[c->pos[dim_y] + c->npc->dir[dim_y]]
                          [c->pos[dim_x] + c->npc->dir[dim_x]]) {
    dest[dim_x] = c->pos[dim_x] + c->npc->dir[dim_x];
    dest[dim_y] = c->pos[dim_y] + c->npc->dir[dim_y];
  }
}

static void move_wanderer_func(character_t *c, pair_t dest){
  dest[dim_x] = c->pos[dim_x];
  dest[dim_y] = c->pos[dim_y];

  if ((world.cur_map->map[c->pos[dim_y] + c->npc->dir[dim_y]]
                         [c->pos[dim_x] + c->npc->dir[dim_x]] !=
       world.cur_map->map[c->pos[dim_y]][c->pos[dim_x]]) ||
      world.cur_map->cmap[c->pos[dim_y] + c->npc->dir[dim_y]]
                         [c->pos[dim_x] + c->npc->dir[dim_x]]) {
    rand_dir(c->npc->dir);
  }

  if ((world.cur_map->map[c->pos[dim_y] + c->npc->dir[dim_y]]
                         [c->pos[dim_x] + c->npc->dir[dim_x]] ==
       world.cur_map->map[c->pos[dim_y]][c->pos[dim_x]]) &&
      !world.cur_map->cmap[c->pos[dim_y] + c->npc->dir[dim_y]]
                          [c->pos[dim_x] + c->npc->dir[dim_x]]) {
    dest[dim_x] = c->pos[dim_x] + c->npc->dir[dim_x];
    dest[dim_y] = c->pos[dim_y] + c->npc->dir[dim_y];
  }
}

static void move_sentry_func(character_t *c, pair_t dest){
  dest[dim_x] = c->pos[dim_x];
  dest[dim_y] = c->pos[dim_y];
}

static void move_walker_func(character_t *c, pair_t dest){
  dest[dim_x] = c->pos[dim_x];
  dest[dim_y] = c->pos[dim_y];

  if ((move_cost[char_other][world.cur_map->map[c->pos[dim_y] +
                                                c->npc->dir[dim_y]]
                                               [c->pos[dim_x] +
                                                c->npc->dir[dim_x]]] ==
       INT_MAX) || world.cur_map->cmap[c->pos[dim_y] + c->npc->dir[dim_y]]
                                      [c->pos[dim_x] + c->npc->dir[dim_x]]) {
    c->npc->dir[dim_x] *= -1;
    c->npc->dir[dim_y] *= -1;
  }

  if ((move_cost[char_other][world.cur_map->map[c->pos[dim_y] +
                                                c->npc->dir[dim_y]]
                                               [c->pos[dim_x] +
                                                c->npc->dir[dim_x]]] !=
       INT_MAX) &&
      !world.cur_map->cmap[c->pos[dim_y] + c->npc->dir[dim_y]]
                          [c->pos[dim_x] + c->npc->dir[dim_x]]) {
    dest[dim_x] = c->pos[dim_x] + c->npc->dir[dim_x];
    dest[dim_y] = c->pos[dim_y] + c->npc->dir[dim_y];
  }
}

static void move_pc_func(character_t *c, pair_t dest){
  dest[dim_x] = c->pos[dim_x];
  dest[dim_y] = c->pos[dim_y];
}

void (*move_func[num_movement_types])(character_t *, pair_t) = {
  move_hiker_func,
  move_rival_func,
  move_pacer_func,
  move_wanderer_func,
  move_sentry_func,
  move_walker_func,
  move_pc_func,
};

static int32_t path_cmp(const void *key, const void *with) {
  return ((path_t *) key)->cost - ((path_t *) with)->cost;
}

static int32_t edge_penalty(int8_t x, int8_t y){
  return (x == 1 || y == 1 || x == MAP_X - 2 || y == MAP_Y - 2) ? 2 : 1;
}

static void dijkstra_path(map_t *m, pair_t from, pair_t to){
  static path_t path[MAP_Y][MAP_X], *p;
  static uint32_t initialized = 0;
  heap_t h;
  uint32_t x, y;

  if (!initialized) {
    for (y = 0; y < MAP_Y; y++) {
      for (x = 0; x < MAP_X; x++) {
        path[y][x].pos[dim_y] = y;
        path[y][x].pos[dim_x] = x;
      }
    }
    initialized = 1;
  }
  
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      path[y][x].cost = INT_MAX;
    }
  }

  path[from[dim_y]][from[dim_x]].cost = 0;

  heap_init(&h, path_cmp, NULL);

  for (y = 1; y < MAP_Y - 1; y++) {
    for (x = 1; x < MAP_X - 1; x++) {
      path[y][x].hn = heap_insert(&h, &path[y][x]);
    }
  }

  while ((p = heap_remove_min(&h))) {
    p->hn = NULL;

    if ((p->pos[dim_y] == to[dim_y]) && p->pos[dim_x] == to[dim_x]) {
      for (x = to[dim_x], y = to[dim_y];
           (x != from[dim_x]) || (y != from[dim_y]);
           p = &path[y][x], x = p->from[dim_x], y = p->from[dim_y]) {
        mapxy(x, y) = ter_path;
        heightxy(x, y) = 0;
      }
      heap_delete(&h);
      return;
    }

    if ((path[p->pos[dim_y] - 1][p->pos[dim_x]    ].hn) &&
        (path[p->pos[dim_y] - 1][p->pos[dim_x]    ].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x], p->pos[dim_y] - 1)))) {
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].cost =
        ((p->cost + heightpair(p->pos)) *
         edge_penalty(p->pos[dim_x], p->pos[dim_y] - 1));
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1]
                                           [p->pos[dim_x]    ].hn);
    }
    if ((path[p->pos[dim_y]    ][p->pos[dim_x] - 1].hn) &&
        (path[p->pos[dim_y]    ][p->pos[dim_x] - 1].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x] - 1, p->pos[dim_y])))) {
      path[p->pos[dim_y]][p->pos[dim_x] - 1].cost =
        ((p->cost + heightpair(p->pos)) *
         edge_penalty(p->pos[dim_x] - 1, p->pos[dim_y]));
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]    ]
                                           [p->pos[dim_x] - 1].hn);
    }
    if ((path[p->pos[dim_y]    ][p->pos[dim_x] + 1].hn) &&
        (path[p->pos[dim_y]    ][p->pos[dim_x] + 1].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x] + 1, p->pos[dim_y])))) {
      path[p->pos[dim_y]][p->pos[dim_x] + 1].cost =
        ((p->cost + heightpair(p->pos)) *
         edge_penalty(p->pos[dim_x] + 1, p->pos[dim_y]));
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]    ]
                                           [p->pos[dim_x] + 1].hn);
    }
    if ((path[p->pos[dim_y] + 1][p->pos[dim_x]    ].hn) &&
        (path[p->pos[dim_y] + 1][p->pos[dim_x]    ].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x], p->pos[dim_y] + 1)))) {
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].cost =
        ((p->cost + heightpair(p->pos)) *
         edge_penalty(p->pos[dim_x], p->pos[dim_y] + 1));
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1]
                                           [p->pos[dim_x]    ].hn);
    }
  }
}

static int build_paths(map_t *m){
  pair_t from, to;

  if (m->e != -1 && m->w != -1) {
    from[dim_x] = 1;
    to[dim_x] = MAP_X - 2;
    from[dim_y] = m->w;
    to[dim_y] = m->e;

    dijkstra_path(m, from, to);
  }

  if (m->n != -1 && m->s != -1) {
    from[dim_y] = 1;
    to[dim_y] = MAP_Y - 2;
    from[dim_x] = m->n;
    to[dim_x] = m->s;

    dijkstra_path(m, from, to);
  }

  if (m->e == -1) {
    if (m->s == -1) {
      from[dim_x] = 1;
      from[dim_y] = m->w;
      to[dim_x] = m->n;
      to[dim_y] = 1;
    } else {
      from[dim_x] = 1;
      from[dim_y] = m->w;
      to[dim_x] = m->s;
      to[dim_y] = MAP_Y - 2;
    }

    dijkstra_path(m, from, to);
  }

  if (m->w == -1) {
    if (m->s == -1) {
      from[dim_x] = MAP_X - 2;
      from[dim_y] = m->e;
      to[dim_x] = m->n;
      to[dim_y] = 1;
    } else {
      from[dim_x] = MAP_X - 2;
      from[dim_y] = m->e;
      to[dim_x] = m->s;
      to[dim_y] = MAP_Y - 2;
    }

    dijkstra_path(m, from, to);
  }

  if (m->n == -1) {
    if (m->e == -1) {
      from[dim_x] = 1;
      from[dim_y] = m->w;
      to[dim_x] = m->s;
      to[dim_y] = MAP_Y - 2;
    } else {
      from[dim_x] = MAP_X - 2;
      from[dim_y] = m->e;
      to[dim_x] = m->s;
      to[dim_y] = MAP_Y - 2;
    }

    dijkstra_path(m, from, to);
  }

  if (m->s == -1) {
    if (m->e == -1) {
      from[dim_x] = 1;
      from[dim_y] = m->w;
      to[dim_x] = m->n;
      to[dim_y] = 1;
    } else {
      from[dim_x] = MAP_X - 2;
      from[dim_y] = m->e;
      to[dim_x] = m->n;
      to[dim_y] = 1;
    }

    dijkstra_path(m, from, to);
  }

  return 0;
}

static int gaussian[5][5] = {
  {  1,  4,  7,  4,  1 },
  {  4, 16, 26, 16,  4 },
  {  7, 26, 41, 26,  7 },
  {  4, 16, 26, 16,  4 },
  {  1,  4,  7,  4,  1 }
};

static int smooth_height(map_t *m){
  int32_t i, x, y;
  int32_t s, t, p, q;
  queue_node_t *head, *tail, *tmp;
  uint8_t height[MAP_Y][MAP_X];

  memset(&height, 0, sizeof (height));

  /* Seed with some values */
  for (i = 1; i < 255; i += 20) {
    do {
      x = rand() % MAP_X;
      y = rand() % MAP_Y;
    } while (height[y][x]);
    height[y][x] = i;
    if (i == 1) {
      head = tail = malloc(sizeof (*tail));
    } else {
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
    }
    tail->next = NULL;
    tail->x = x;
    tail->y = y;
  }

  /* Diffuse the vaules to fill the space */
  while (head) {
    x = head->x;
    y = head->y;
    i = height[y][x];

    if (x - 1 >= 0 && y - 1 >= 0 && !height[y - 1][x - 1]) {
      height[y - 1][x - 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y - 1;
    }
    if (x - 1 >= 0 && !height[y][x - 1]) {
      height[y][x - 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y;
    }
    if (x - 1 >= 0 && y + 1 < MAP_Y && !height[y + 1][x - 1]) {
      height[y + 1][x - 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y + 1;
    }
    if (y - 1 >= 0 && !height[y - 1][x]) {
      height[y - 1][x] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x;
      tail->y = y - 1;
    }
    if (y + 1 < MAP_Y && !height[y + 1][x]) {
      height[y + 1][x] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x;
      tail->y = y + 1;
    }
    if (x + 1 < MAP_X && y - 1 >= 0 && !height[y - 1][x + 1]) {
      height[y - 1][x + 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y - 1;
    }
    if (x + 1 < MAP_X && !height[y][x + 1]) {
      height[y][x + 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y;
    }
    if (x + 1 < MAP_X && y + 1 < MAP_Y && !height[y + 1][x + 1]) {
      height[y + 1][x + 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y + 1;
    }

    tmp = head;
    head = head->next;
    free(tmp);
  }

  /* And smooth it a bit with a gaussian convolution */
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      for (s = t = p = 0; p < 5; p++) {
        for (q = 0; q < 5; q++) {
          if (y + (p - 2) >= 0 && y + (p - 2) < MAP_Y &&
              x + (q - 2) >= 0 && x + (q - 2) < MAP_X) {
            s += gaussian[p][q];
            t += height[y + (p - 2)][x + (q - 2)] * gaussian[p][q];
          }
        }
      }
      m->height[y][x] = t / s;
    }
  }

  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      for (s = t = p = 0; p < 5; p++) {
        for (q = 0; q < 5; q++) {
          if (y + (p - 2) >= 0 && y + (p - 2) < MAP_Y &&
              x + (q - 2) >= 0 && x + (q - 2) < MAP_X) {
            s += gaussian[p][q];
            t += height[y + (p - 2)][x + (q - 2)] * gaussian[p][q];
          }
        }
      }
      m->height[y][x] = t / s;
    }
  }
  return 0;
}

static void find_building_location(map_t *m, pair_t p){
  do {
    p[dim_x] = rand() % (MAP_X - 5) + 3;
    p[dim_y] = rand() % (MAP_Y - 10) + 5;

    if ((((mapxy(p[dim_x] - 1, p[dim_y]    ) == ter_path)     &&
          (mapxy(p[dim_x] - 1, p[dim_y] + 1) == ter_path))    ||
         ((mapxy(p[dim_x] + 2, p[dim_y]    ) == ter_path)     &&
          (mapxy(p[dim_x] + 2, p[dim_y] + 1) == ter_path))    ||
         ((mapxy(p[dim_x]    , p[dim_y] - 1) == ter_path)     &&
          (mapxy(p[dim_x] + 1, p[dim_y] - 1) == ter_path))    ||
         ((mapxy(p[dim_x]    , p[dim_y] + 2) == ter_path)     &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 2) == ter_path)))   &&
        (((mapxy(p[dim_x]    , p[dim_y]    ) != ter_mart)     &&
          (mapxy(p[dim_x]    , p[dim_y]    ) != ter_center)   &&
          (mapxy(p[dim_x] + 1, p[dim_y]    ) != ter_mart)     &&
          (mapxy(p[dim_x] + 1, p[dim_y]    ) != ter_center)   &&
          (mapxy(p[dim_x]    , p[dim_y] + 1) != ter_mart)     &&
          (mapxy(p[dim_x]    , p[dim_y] + 1) != ter_center)   &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_mart)     &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_center))) &&
        (((mapxy(p[dim_x]    , p[dim_y]    ) != ter_path)     &&
          (mapxy(p[dim_x] + 1, p[dim_y]    ) != ter_path)     &&
          (mapxy(p[dim_x]    , p[dim_y] + 1) != ter_path)     &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_path)))) {
          break;
    }
  } while (1);
}

static int place_pokemart(map_t *m){
  pair_t p;

  find_building_location(m, p);

  mapxy(p[dim_x]    , p[dim_y]    ) = ter_mart;
  mapxy(p[dim_x] + 1, p[dim_y]    ) = ter_mart;
  mapxy(p[dim_x]    , p[dim_y] + 1) = ter_mart;
  mapxy(p[dim_x] + 1, p[dim_y] + 1) = ter_mart;

  return 0;
}

static int place_center(map_t *m){
  pair_t p;

  find_building_location(m, p);

  mapxy(p[dim_x]    , p[dim_y]    ) = ter_center;
  mapxy(p[dim_x] + 1, p[dim_y]    ) = ter_center;
  mapxy(p[dim_x]    , p[dim_y] + 1) = ter_center;
  mapxy(p[dim_x] + 1, p[dim_y] + 1) = ter_center;

  return 0;
}

static int map_terrain(map_t *m, int8_t n, int8_t s, int8_t e, int8_t w){
  int32_t i, x, y;
  queue_node_t *head, *tail, *tmp;
  int num_grass, num_clearing, num_mountain, num_forest, num_total;
  terrain_type_t type;
  int added_current = 0;
  
  num_grass = rand() % 4 + 2;
  num_clearing = rand() % 4 + 2;
  num_mountain = rand() % 2 + 1;
  num_forest = rand() % 2 + 1;
  num_total = num_grass + num_clearing + num_mountain + num_forest;

  memset(&m->map, 0, sizeof (m->map));

  /* Seed with some values */
  for (i = 0; i < num_total; i++) {
    do {
      x = rand() % MAP_X;
      y = rand() % MAP_Y;
    } while (m->map[y][x]);
    if (i == 0) {
      type = ter_grass;
    } else if (i == num_grass) {
      type = ter_clearing;
    } else if (i == num_grass + num_clearing) {
      type = ter_mountain;
    } else if (i == num_grass + num_clearing + num_mountain) {
      type = ter_forest;
    }
    m->map[y][x] = type;
    if (i == 0) {
      head = tail = malloc(sizeof (*tail));
    } else {
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
    }
    tail->next = NULL;
    tail->x = x;
    tail->y = y;
  }

  /* Diffuse the vaules to fill the space */
  while (head) {
    x = head->x;
    y = head->y;
    i = m->map[y][x];
    
    if (x - 1 >= 0 && !m->map[y][x - 1]) {
      if ((rand() % 100) < 80) {
        m->map[y][x - 1] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x - 1;
        tail->y = y;
      } else if (!added_current) {
        added_current = 1;
        m->map[y][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (y - 1 >= 0 && !m->map[y - 1][x]) {
      if ((rand() % 100) < 20) {
        m->map[y - 1][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y - 1;
      } else if (!added_current) {
        added_current = 1;
        m->map[y][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (y + 1 < MAP_Y && !m->map[y + 1][x]) {
      if ((rand() % 100) < 20) {
        m->map[y + 1][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y + 1;
      } else if (!added_current) {
        added_current = 1;
        m->map[y][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (x + 1 < MAP_X && !m->map[y][x + 1]) {
      if ((rand() % 100) < 80) {
        m->map[y][x + 1] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x + 1;
        tail->y = y;
      } else if (!added_current) {
        added_current = 1;
        m->map[y][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    added_current = 0;
    tmp = head;
    head = head->next;
    free(tmp);
  }

  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      if (y == 0 || y == MAP_Y - 1 ||
          x == 0 || x == MAP_X - 1) {
        mapxy(x, y) = ter_boulder;
      }
    }
  }

  m->n = n;
  m->s = s;
  m->e = e;
  m->w = w;

  if (n != -1) {
    mapxy(n,         0        ) = ter_path;
    mapxy(n,         1        ) = ter_path;
  }
  if (s != -1) {
    mapxy(s,         MAP_Y - 1) = ter_path;
    mapxy(s,         MAP_Y - 2) = ter_path;
  }
  if (w != -1) {
    mapxy(0,         w        ) = ter_path;
    mapxy(1,         w        ) = ter_path;
  }
  if (e != -1) {
    mapxy(MAP_X - 1, e        ) = ter_path;
    mapxy(MAP_X - 2, e        ) = ter_path;
  }

  return 0;
}

static int place_boulders(map_t *m){
  int i;
  int x, y;

  for (i = 0; i < MIN_BOULDERS || rand() % 100 < BOULDER_PROB; i++) {
    y = rand() % (MAP_Y - 2) + 1;
    x = rand() % (MAP_X - 2) + 1;
    if (m->map[y][x] != ter_forest && m->map[y][x] != ter_path) {
      m->map[y][x] = ter_boulder;
    }
  }

  return 0;
}

static int place_trees(map_t *m){
  int i;
  int x, y;
  
  for (i = 0; i < MIN_TREES || rand() % 100 < TREE_PROB; i++) {
    y = rand() % (MAP_Y - 2) + 1;
    x = rand() % (MAP_X - 2) + 1;
    if (m->map[y][x] != ter_mountain && m->map[y][x] != ter_path) {
      m->map[y][x] = ter_tree;
    }
  }

  return 0;
}

void rand_pos(pair_t pos){
  pos[dim_x] = (rand() % (MAP_X - 2)) + 1;
  pos[dim_y] = (rand() % (MAP_Y - 2)) + 1;
}

void new_hiker(){
  pair_t pos;
  character_t *c;

  do {
    rand_pos(pos);
  } while (world.hiker_dist[pos[dim_y]][pos[dim_x]] == INT_MAX ||
           world.cur_map->cmap[pos[dim_y]][pos[dim_x]]);

  c = malloc(sizeof (*c));
  c->npc = malloc(sizeof (*c->npc));
  c->pos[dim_y] = pos[dim_y];
  c->pos[dim_x] = pos[dim_x];
  c->npc->ctype = char_hiker;
  c->npc->mtype = move_hiker;
  c->npc->dir[dim_x] = 0;
  c->npc->dir[dim_y] = 0;
  c->symbol = 'h';
  c->next_turn = 0;
  c->defeated = false;
  heap_insert(&world.cur_map->turn, c);

}

void new_rival(){
  pair_t pos;
  character_t *c;

  do {
    rand_pos(pos);
  } while (world.rival_dist[pos[dim_y]][pos[dim_x]] == INT_MAX ||
           world.rival_dist[pos[dim_y]][pos[dim_x]] < 0        ||
           world.cur_map->cmap[pos[dim_y]][pos[dim_x]]);

  c = malloc(sizeof (*c));
  c->npc = malloc(sizeof (*c->npc));
  c->pos[dim_y] = pos[dim_y];
  c->pos[dim_x] = pos[dim_x];
  c->npc->ctype = char_rival;
  c->npc->mtype = move_rival;
  c->npc->dir[dim_x] = 0;
  c->npc->dir[dim_y] = 0;
  c->symbol = 'r';
  c->next_turn = 0;
  c->defeated = false;
  heap_insert(&world.cur_map->turn, c);
}

void new_char_other(){
  pair_t pos;
  character_t *c;

  do {
    rand_pos(pos);
  } while (world.rival_dist[pos[dim_y]][pos[dim_x]] == INT_MAX ||
           world.rival_dist[pos[dim_y]][pos[dim_x]] < 0        ||
           world.cur_map->cmap[pos[dim_y]][pos[dim_x]]);

  c = malloc(sizeof (*c));
  c->npc = malloc(sizeof (*c->npc));
  c->pos[dim_y] = pos[dim_y];
  c->pos[dim_x] = pos[dim_x];
  c->npc->ctype = char_other;
  switch (rand() % 4) {
  case 0:
    c->npc->mtype = move_pace;
    c->symbol = 'p';
    break;
  case 1:
    c->npc->mtype = move_wander;
    c->symbol = 'w';
    break;
  case 2:
    c->npc->mtype = move_sentry;
    c->symbol = 's';
    break;
  case 3:
    c->npc->mtype = move_walk;
    c->symbol = 'n';
    break;
  }
  rand_dir(c->npc->dir);
  c->next_turn = 0;
  c->defeated = false;
  heap_insert(&world.cur_map->turn, c);
}

void place_characters(){
  int num_trainers = 2;

  //Always place a hiker and a rival, then place a random number of others
  new_hiker();
  new_rival();
  do {
    //higher probability of non- hikers and rivals
    switch(rand() % 10) {
    case 0:
      new_hiker();
      break;
    case 1:
     new_rival();
      break;
    default:
      new_char_other();
      break;
    }
  } while (++num_trainers < MIN_TRAINERS ||
           ((rand() % 100) < ADD_TRAINER_PROB));
}

int32_t cmp_char_turns(const void *key, const void *with){
  return ((character_t *) key)->next_turn - ((character_t *) with)->next_turn;
}

void delete_character(void *v){
  if (v == &world.pc) {
    free(world.pc.pc);
  } else {
    free(((character_t *) v)->npc);
    free(v);
  }
}

void init_pc(){
  int x, y;

  do {
    x = rand() % (MAP_X - 2) + 1;
    y = rand() % (MAP_Y - 2) + 1;
  } while (world.cur_map->map[y][x] != ter_path);

  world.pc.pos[dim_x] = x;
  world.pc.pos[dim_y] = y;
  world.pc.symbol = '@';
  world.pc.pc = malloc(sizeof (*world.pc.pc));

  world.cur_map->cmap[y][x] = &world.pc;
  world.pc.next_turn = 0;

  heap_insert(&world.cur_map->turn, &world.pc);
}

#define ter_cost(x, y, c) move_cost[c][m->map[y][x]]

static int32_t hiker_cmp(const void *key, const void *with) {
  return (world.hiker_dist[((path_t *) key)->pos[dim_y]]
                          [((path_t *) key)->pos[dim_x]] -
          world.hiker_dist[((path_t *) with)->pos[dim_y]]
                          [((path_t *) with)->pos[dim_x]]);
}

static int32_t rival_cmp(const void *key, const void *with) {
  return (world.rival_dist[((path_t *) key)->pos[dim_y]]
                          [((path_t *) key)->pos[dim_x]] -
          world.rival_dist[((path_t *) with)->pos[dim_y]]
                          [((path_t *) with)->pos[dim_x]]);
}

void pathfind(map_t *m){
  heap_t h;
  uint32_t x, y;
  static path_t p[MAP_Y][MAP_X], *c;
  static uint32_t initialized = 0;

  if (!initialized) {
    initialized = 1;
    for (y = 0; y < MAP_Y; y++) {
      for (x = 0; x < MAP_X; x++) {
        p[y][x].pos[dim_y] = y;
        p[y][x].pos[dim_x] = x;
      }
    }
  }

  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      world.hiker_dist[y][x] = world.rival_dist[y][x] = INT_MAX;
    }
  }
  world.hiker_dist[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = 
    world.rival_dist[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = 0;

  heap_init(&h, hiker_cmp, NULL);

  for (y = 1; y < MAP_Y - 1; y++) {
    for (x = 1; x < MAP_X - 1; x++) {
      if (ter_cost(x, y, char_hiker) != INT_MAX) {
        p[y][x].hn = heap_insert(&h, &p[y][x]);
      } else {
        p[y][x].hn = NULL;
      }
    }
  }

  while ((c = heap_remove_min(&h))) {
    c->hn = NULL;
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x] - 1].hn) &&
        (world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x] - 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x] - 1] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x]    ].hn) &&
        (world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x]    ] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x]    ] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x]    ].hn);
    }
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x] + 1].hn) &&
        (world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x] + 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x] + 1] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x] + 1].hn);
    }
    if ((p[c->pos[dim_y]    ][c->pos[dim_x] - 1].hn) &&
        (world.hiker_dist[c->pos[dim_y]    ][c->pos[dim_x] - 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y]    ][c->pos[dim_x] - 1] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y]    ][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y]    ][c->pos[dim_x] + 1].hn) &&
        (world.hiker_dist[c->pos[dim_y]    ][c->pos[dim_x] + 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y]    ][c->pos[dim_x] + 1] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y]    ][c->pos[dim_x] + 1].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x] - 1].hn) &&
        (world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x] - 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x] - 1] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x]    ].hn) &&
        (world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x]    ] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x]    ] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x]    ].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x] + 1].hn) &&
        (world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x] + 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x] + 1] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x] + 1].hn);
    }
  }
  heap_delete(&h);

  heap_init(&h, rival_cmp, NULL);

  for (y = 1; y < MAP_Y - 1; y++) {
    for (x = 1; x < MAP_X - 1; x++) {
      if (ter_cost(x, y, char_rival) != INT_MAX) {
        p[y][x].hn = heap_insert(&h, &p[y][x]);
      } else {
        p[y][x].hn = NULL;
      }
    }
  }

  while ((c = heap_remove_min(&h))) {
    c->hn = NULL;
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x] - 1].hn) &&
        (world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x] - 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x] - 1] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x]    ].hn) &&
        (world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x]    ] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x]    ] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x]    ].hn);
    }
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x] + 1].hn) &&
        (world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x] + 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x] + 1] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x] + 1].hn);
    }
    if ((p[c->pos[dim_y]    ][c->pos[dim_x] - 1].hn) &&
        (world.rival_dist[c->pos[dim_y]    ][c->pos[dim_x] - 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y]    ][c->pos[dim_x] - 1] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y]    ][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y]    ][c->pos[dim_x] + 1].hn) &&
        (world.rival_dist[c->pos[dim_y]    ][c->pos[dim_x] + 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y]    ][c->pos[dim_x] + 1] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y]    ][c->pos[dim_x] + 1].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x] - 1].hn) &&
        (world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x] - 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x] - 1] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x]    ].hn) &&
        (world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x]    ] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x]    ] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x]    ].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x] + 1].hn) &&
        (world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x] + 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x] + 1] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x] + 1].hn);
    }
  }
  heap_delete(&h);
}

static int new_map(){
  int d, p;
  int e, w, n, s;
  int x, y;
  
  if (world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x]]) {
    world.cur_map = world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x]];
    return 0;
  }

  world.cur_map                                             =
    world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x]] =
    malloc(sizeof (*world.cur_map));

  smooth_height(world.cur_map);
  
  if (!world.cur_idx[dim_y]) {
    n = -1;
  } else if (world.world[world.cur_idx[dim_y] - 1][world.cur_idx[dim_x]]) {
    n = world.world[world.cur_idx[dim_y] - 1][world.cur_idx[dim_x]]->s;
  } else {
    n = 1 + rand() % (MAP_X - 2);
  }
  if (world.cur_idx[dim_y] == WORLD_SIZE - 1) {
    s = -1;
  } else if (world.world[world.cur_idx[dim_y] + 1][world.cur_idx[dim_x]]) {
    s = world.world[world.cur_idx[dim_y] + 1][world.cur_idx[dim_x]]->n;
  } else  {
    s = 1 + rand() % (MAP_X - 2);
  }
  if (!world.cur_idx[dim_x]) {
    w = -1;
  } else if (world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x] - 1]) {
    w = world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x] - 1]->e;
  } else {
    w = 1 + rand() % (MAP_Y - 2);
  }
  if (world.cur_idx[dim_x] == WORLD_SIZE - 1) {
    e = -1;
  } else if (world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x] + 1]) {
    e = world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x] + 1]->w;
  } else {
    e = 1 + rand() % (MAP_Y - 2);
  }
  
  map_terrain(world.cur_map, n, s, e, w);
     
  place_boulders(world.cur_map);
  place_trees(world.cur_map);
  build_paths(world.cur_map);
  d = (abs(world.cur_idx[dim_x] - (WORLD_SIZE / 2)) +
       abs(world.cur_idx[dim_y] - (WORLD_SIZE / 2)));
  p = d > 200 ? 5 : (50 - ((45 * d) / 200));

  if ((rand() % 100) < p || !d) {
    place_pokemart(world.cur_map);
  }
  if ((rand() % 100) < p || !d) {
    place_center(world.cur_map);
  }

  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      world.cur_map->cmap[y][x] = NULL;
    }
  }

  heap_init(&world.cur_map->turn, cmp_char_turns, delete_character);

  init_pc();
  pathfind(world.cur_map);
  place_characters();

  return 0;
}

static void print_map(){
    int x, y;
    int default_reached = 0;

    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_BLACK);
    init_pair(5, COLOR_BLUE, COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);

    for (y = 0; y < MAP_Y; y++) {
        for (x = 0; x < MAP_X; x++) {
            if (world.cur_map->cmap[y][x]) {
                mvaddch(y+1, x, world.cur_map->cmap[y][x]->symbol);
            } else {
                switch (world.cur_map->map[y][x]) {
                case ter_boulder:
                case ter_mountain:
                    attron(COLOR_PAIR(1));
                    mvaddch(y+1, x, '%');
                    attroff(COLOR_PAIR(1));
                    break;
                case ter_tree:
                case ter_forest:
                    attron(COLOR_PAIR(2));
                    mvaddch(y+1, x, '^');
                    attroff(COLOR_PAIR(2));
                    break;
                case ter_path:
                    attron(COLOR_PAIR(6));
                    mvaddch(y+1, x, '#');
                    attroff(COLOR_PAIR(6));
                    break;
                case ter_mart:
                    attron(COLOR_PAIR(4));
                    mvaddch(y+1, x, 'M');
                    attroff(COLOR_PAIR(4));
                    break;
                case ter_center:
                    attron(COLOR_PAIR(4));
                    mvaddch(y+1, x, 'C');
                    attroff(COLOR_PAIR(4));
                    break;
                case ter_grass:
                    attron(COLOR_PAIR(3));
                    mvaddch(y+1, x, ':');
                    attron(COLOR_PAIR(3));
                    break;
                case ter_clearing:
                    attron(COLOR_PAIR(5));
                    mvaddch(y+1, x, '.');
                    attron(COLOR_PAIR(5));
                    break;
                default:
                    default_reached = 1;
                break;
                }
            }
        }
    }

    if (default_reached) {
        fprintf(stderr, "Default reached in %s\n", __FUNCTION__);
    }
  refresh();
}

// The world is global because of its size, so init_world is parameterless
void init_world(){
  world.cur_idx[dim_x] = world.cur_idx[dim_y] = WORLD_SIZE / 2;
  new_map();
}

void delete_world(){
  int x, y;
  heap_delete(&world.cur_map->turn);

  for (y = 0; y < WORLD_SIZE; y++) {
    for (x = 0; x < WORLD_SIZE; x++) {
      if (world.world[y][x]) {
        free(world.world[y][x]);
        world.world[y][x] = NULL;
      }
    }
  }
}

void wipe_screen(){
  int x, y;
  for (y = 0; y < MAP_Y; y++)
  {
    for (x = 0; x < MAP_X; x++)
    {
      mvprintw(y+1, x, " ");
    }
  }
}

void enter_building(){
    clear();
    printw("Hello adventurer!\n");
    printw("Press '<' to exit");
     refresh();
    char input;
    do{
        input = getchar();
    } while(input != '<');

    clear();
    wipe_screen();
    refresh();
}

void enter_battle(){
    clear();
    printw("Battle!!!!!\n");
    printw("use esc to escape");
    refresh();
    char input;
    do{
        input = getchar();
    } while(input != 27);

    clear();
    wipe_screen();
    refresh();
}

void trainer_list(){
    int x, y;
    int num_trainers = 0;
    for(y = 0; y < MAP_Y; y++){
        for(x = 0; x < MAP_X; x++){
        if(world.cur_map->cmap[y][x]){
            if(world.cur_map->cmap[y][x]->symbol != '@'){
            num_trainers++;
            }
        }
        }
    }

    character_t trainers[num_trainers];
    int count = 0;
    for(y = 0; y < MAP_Y; y++){
        for(x = 0; x < MAP_X; x++){
            if(world.cur_map->cmap[y][x]){
                if(world.cur_map->cmap[y][x]->symbol != '@'){
                trainers[count].symbol = world.cur_map->cmap[y][x]->symbol;
                trainers[count].pos[dim_x] = x;
                trainers[count].pos[dim_y] = y;
                count++;
                }
            }
        }
    }

    char input;
    printw("Number Trainers = %d", num_trainers);
    refresh();
    move(0,0);
    int i;
    int line = 2;
    do{
        line = 2;
        for(i = 0; i<num_trainers; i++){
        move(line, 0);
        int x_dist = trainers[i].pos[dim_x] - world.pc.pos[dim_x];
        int y_dist = trainers[i].pos[dim_y] - world.pc.pos[dim_y];
        mvprintw(line, 0, "%c, %d %s %d %s", trainers[i].symbol, abs(y_dist), y_dist >= 0 ? "n" : "s", abs(x_dist), x_dist >= 0 ? "e" : "w");
        line++;
        refresh();
        }
        input = getchar();

    } while(input != 27);  
}

int Check(character_t *c, pair_t next){
    if(world.cur_map->map[next[dim_y]][next[dim_x]] == ter_center){
        enter_building();
        return 0;
    }else if(world.cur_map->map[next[dim_y]][next[dim_x]] == ter_mart){
        enter_building();
        return 0;
    }else if(world.cur_map->cmap[next[dim_y]][next[dim_x]] && world.cur_map->cmap[next[dim_y]][next[dim_x]]->symbol != '@'){
        enter_battle();
        world.cur_map->cmap[next[dim_y]][next[dim_x]]->defeated = true;
        world.cur_map->cmap[next[dim_y]][next[dim_x]] = NULL;
        return 1;
    } else if(move_cost[0][world.cur_map->map[next[dim_y]][next[dim_x]]] == INT_MAX){
        mvprintw(22, 0, "Invaild movement");
        return 1;
    }
    return 0;
}

void pc_movement(character_t *pc, uint32_t input){
    clear();
    pair_t next;
    next[dim_x] = pc->pos[dim_x];
    next[dim_y] = pc->pos[dim_y];
    switch(input){
        case '9':
        case 'u':
        next[dim_x]++;
        next[dim_y]--;
        if(Check(pc, next) != 0){
            next[dim_x]--;
            next[dim_y]++; 
        }
        break;
        case '8':
        case 'k':
        next[dim_y]--;
        if(Check(pc, next) != 0){
            next[dim_y]++; 
        }
        break;
        case '7':
        case 'y':
        next[dim_x]--;
        next[dim_y]--;
        if(Check(pc, next) != 0){
            next[dim_x]++;
            next[dim_y]++; 
        }
        break;
        case '6':
        case 'l':
        next[dim_x]++;
        if(Check(pc, next) != 0){
            next[dim_x]--;
        }
        break;
        case '4':
        case 'h':
        next[dim_x]--;
        if(Check(pc, next) != 0){
            next[dim_x]++;
        }
        break;
        case '3':
        case 'n':
        next[dim_x]++;
        next[dim_y]++;
        if(Check(pc, next) != 0){
            next[dim_x]--;
            next[dim_y]--;
        }
        break;
        case '2':
        case 'j':
        next[dim_y]++;
        if(Check(pc, next) != 0){
            
            next[dim_y]--;
        }
        break;
        case '1':
        case 'b':
        next[dim_x]--;
        next[dim_y]++;
        if(Check(pc, next) != 0){
            next[dim_x]++;
            next[dim_y]--;
        }
        break;
        case '5':
        case ' ':
        break;

        case '>':
        if (world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] == ter_mart || world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] == ter_center){
            enter_building();
        }else{
            mvprintw(22, 0, "please enter a valid entry");
        }
        break;
        default:
        break;
    }
    
    world.cur_map->cmap[pc->pos[dim_y]][pc->pos[dim_x]] = NULL;
    world.cur_map->cmap[next[dim_y]][next[dim_x]] = pc;
    
    if((input == '5' || input == ' ')){
        pc->next_turn += move_cost[0][world.cur_map->map[next[dim_y]][next[dim_x]]];
    } else if((next[dim_x] != pc->pos[dim_x] || next[dim_y] != pc->pos[dim_y])){
        pc->next_turn += move_cost[0][world.cur_map->map[next[dim_y]][next[dim_x]]];
    }
    

    pc->pos[dim_x] = next[dim_x];
    pc->pos[dim_y] = next[dim_y];
    pathfind(world.cur_map);
}

void game_loop(){
    character_t *c;
    pair_t d;

    initscr();
    raw();
    noecho();
    start_color();
    curs_set(0);
    keypad(stdscr, TRUE);
    refresh();

    char input;

    while(input != 'q'){
        print_map();
        c = heap_remove_min(&world.cur_map->turn);
        if (c == &world.pc) {
        input = getchar();
        if(input == 't'){
            clear();
            trainer_list();
            clear();
            refresh();
            print_map();
        }else{
                pc_movement(c, input);
                print_map();  
            }
        } else if(!(c->defeated)){
            move_func[c->npc->mtype](c, d);
            if(world.cur_map->cmap[d[dim_y]][d[dim_x]] == &world.pc){
                enter_battle();
            }
                world.cur_map->cmap[c->pos[dim_y]][c->pos[dim_x]] = NULL;
                world.cur_map->cmap[d[dim_y]][d[dim_x]] = c;
                c->next_turn += move_cost[c->npc->ctype][world.cur_map->map[d[dim_y]]
                                                                            [d[dim_x]]];
                c->pos[dim_y] = d[dim_y];
                c->pos[dim_x] = d[dim_x];
            }
            if(!(c->defeated)){
                heap_insert(&world.cur_map->turn, c);
        }
    }
    endwin();
}

int main(int argc, char *argv[]){
    struct timeval tv;
    uint32_t seed;

    if (argc == 2) {
        seed = atoi(argv[1]);
    } else {
        gettimeofday(&tv, NULL);
        seed = (tv.tv_usec ^ (tv.tv_sec << 20)) & 0xffffffff;
    }
    printf("Using seed: %u\n", seed);
    srand(seed);
    init_world();
    game_loop();
    delete_world();
    
    return 0;
}