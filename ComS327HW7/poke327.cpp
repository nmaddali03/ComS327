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
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "heap.h"
#include "poke327.h"
#include "character.h"
#include "io.h"

using namespace std;

typedef struct queue_node {
  int x, y;
  struct queue_node *next;
} queue_node_t;

world_t world;

pair_t all_dirs[8] = {
  { -1, -1 },
  { -1,  0 },
  { -1,  1 },
  {  0, -1 },
  {  0,  1 },
  {  1, -1 },
  {  1,  0 },
  {  1,  1 },
};

static int32_t path_cmp(const void *key, const void *with) {
  return ((path_t *) key)->cost - ((path_t *) with)->cost;
}

static int32_t edge_penalty(int8_t x, int8_t y)
{
  return (x == 1 || y == 1 || x == MAP_X - 2 || y == MAP_Y - 2) ? 2 : 1;
}

static void dijkstra_path(map_t *m, pair_t from, pair_t to)
{
  static path_t path[MAP_Y][MAP_X], *p;
  //static uint32_t initialized = 0;
  heap_t h;
  //uint32_t x, y;
  int x,y;
  static int initialized = 0;

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

  while ((p = (path_t *)heap_remove_min(&h))) {
    p->hn = NULL;

    if ((p->pos[dim_y] == to[dim_y]) && p->pos[dim_x] == to[dim_x]) {
      for (x = to[dim_x], y = to[dim_y];
           (x != (int16_t)from[dim_x]) || (y != (int16_t)from[dim_y]);
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

static int build_paths(map_t *m)
{
  pair_t from, to;

  /*  printf("%d %d %d %d\n", m->n, m->s, m->e, m->w);*/

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

static int smooth_height(map_t *m)
{
  int32_t i, x, y;
  int32_t s, t, p, q;
  queue_node_t *head, *tail, *tmp;
  /*  FILE *out;*/
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
      head = tail = (queue_node_t *)malloc(sizeof (*tail));
    } else {
      tail->next = (queue_node_t *)malloc(sizeof (*tail));
      tail = tail->next;
    }
    tail->next = NULL;
    tail->x = x;
    tail->y = y;
  }

  /*
  out = fopen("seeded.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&height, sizeof (height), 1, out);
  fclose(out);
  */
  
  /* Diffuse the vaules to fill the space */
  while (head) {
    x = head->x;
    y = head->y;
    i = height[y][x];

    if (x - 1 >= 0 && y - 1 >= 0 && !height[y - 1][x - 1]) {
      height[y - 1][x - 1] = i;
      tail->next = (queue_node_t *)malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y - 1;
    }
    if (x - 1 >= 0 && !height[y][x - 1]) {
      height[y][x - 1] = i;
      tail->next = (queue_node_t *)malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y;
    }
    if (x - 1 >= 0 && y + 1 < MAP_Y && !height[y + 1][x - 1]) {
      height[y + 1][x - 1] = i;
      tail->next = (queue_node_t *)malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y + 1;
    }
    if (y - 1 >= 0 && !height[y - 1][x]) {
      height[y - 1][x] = i;
      tail->next = (queue_node_t *)malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x;
      tail->y = y - 1;
    }
    if (y + 1 < MAP_Y && !height[y + 1][x]) {
      height[y + 1][x] = i;
      tail->next = (queue_node_t *)malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x;
      tail->y = y + 1;
    }
    if (x + 1 < MAP_X && y - 1 >= 0 && !height[y - 1][x + 1]) {
      height[y - 1][x + 1] = i;
      tail->next = (queue_node_t *)malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y - 1;
    }
    if (x + 1 < MAP_X && !height[y][x + 1]) {
      height[y][x + 1] = i;
      tail->next = (queue_node_t *)malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y;
    }
    if (x + 1 < MAP_X && y + 1 < MAP_Y && !height[y + 1][x + 1]) {
      height[y + 1][x + 1] = i;
      tail->next = (queue_node_t *)malloc(sizeof (*tail));
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
  /* Let's do it again, until it's smooth like Kenny G. */
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

  /*
  out = fopen("diffused.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&height, sizeof (height), 1, out);
  fclose(out);

  out = fopen("smoothed.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&m->height, sizeof (m->height), 1, out);
  fclose(out);
  */

  return 0;
}

static void find_building_location(map_t *m, pair_t p)
{
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

static int place_pokemart(map_t *m)
{
  pair_t p;

  find_building_location(m, p);

  mapxy(p[dim_x]    , p[dim_y]    ) = ter_mart;
  mapxy(p[dim_x] + 1, p[dim_y]    ) = ter_mart;
  mapxy(p[dim_x]    , p[dim_y] + 1) = ter_mart;
  mapxy(p[dim_x] + 1, p[dim_y] + 1) = ter_mart;

  return 0;
}

static int place_center(map_t *m)
{  pair_t p;

  find_building_location(m, p);

  mapxy(p[dim_x]    , p[dim_y]    ) = ter_center;
  mapxy(p[dim_x] + 1, p[dim_y]    ) = ter_center;
  mapxy(p[dim_x]    , p[dim_y] + 1) = ter_center;
  mapxy(p[dim_x] + 1, p[dim_y] + 1) = ter_center;

  return 0;
}

static int map_terrain(map_t *m, int8_t n, int8_t s, int8_t e, int8_t w)
{
  int32_t i, x, y;
  queue_node_t *head, *tail, *tmp;
  //  FILE *out;
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
      head = tail = (queue_node_t *)malloc(sizeof (*tail));
    } else {
      tail->next = (queue_node_t *)malloc(sizeof (*tail));
      tail = tail->next;
    }
    tail->next = NULL;
    tail->x = x;
    tail->y = y;
  }

  /*
  out = fopen("seeded.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&m->map, sizeof (m->map), 1, out);
  fclose(out);
  */

  /* Diffuse the vaules to fill the space */
  while (head) {
    x = head->x;
    y = head->y;
    i = m->map[y][x];
    
    if (x - 1 >= 0 && !m->map[y][x - 1]) {
      if ((rand() % 100) < 80) {
        m->map[y][x - 1] = (terrain_type_t)i;
        tail->next = (queue_node_t *)malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x - 1;
        tail->y = y;
      } else if (!added_current) {
        added_current = 1;
        m->map[y][x] = (terrain_type_t)i;
        tail->next = (queue_node_t *)malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (y - 1 >= 0 && !m->map[y - 1][x]) {
      if ((rand() % 100) < 20) {
        m->map[y - 1][x] = (terrain_type_t)i;
        tail->next = (queue_node_t *)malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y - 1;
      } else if (!added_current) {
        added_current = 1;
        m->map[y][x] = (terrain_type_t)i;
        tail->next = (queue_node_t *)malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (y + 1 < MAP_Y && !m->map[y + 1][x]) {
      if ((rand() % 100) < 20) {
        m->map[y + 1][x] = (terrain_type_t)i;
        tail->next = (queue_node_t *)malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y + 1;
      } else if (!added_current) {
        added_current = 1;
        m->map[y][x] = (terrain_type_t)i;
        tail->next = (queue_node_t *)malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (x + 1 < MAP_X && !m->map[y][x + 1]) {
      if ((rand() % 100) < 80) {
        m->map[y][x + 1] = (terrain_type_t)i;
        tail->next = (queue_node_t *)malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x + 1;
        tail->y = y;
      } else if (!added_current) {
        added_current = 1;
        m->map[y][x] = (terrain_type_t)i;
        tail->next = (queue_node_t *)malloc(sizeof (*tail));
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

  /*
  out = fopen("diffused.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&m->map, sizeof (m->map), 1, out);
  fclose(out);
  */
  
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
    mapxy(n,         0        ) = ter_exit;
    mapxy(n,         1        ) = ter_path;
  }
  if (s != -1) {
    mapxy(s,         MAP_Y - 1) = ter_exit;
    mapxy(s,         MAP_Y - 2) = ter_path;
  }
  if (w != -1) {
    mapxy(0,         w        ) = ter_exit;
    mapxy(1,         w        ) = ter_path;
  }
  if (e != -1) {
    mapxy(MAP_X - 1, e        ) = ter_exit;
    mapxy(MAP_X - 2, e        ) = ter_path;
  }

  return 0;
}

static int place_boulders(map_t *m)
{
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

static int place_trees(map_t *m)
{
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

void rand_pos(pair_t pos)
{
  pos[dim_x] = (rand() % (MAP_X - 2)) + 1;
  pos[dim_y] = (rand() % (MAP_Y - 2)) + 1;
}

void new_hiker()
{
  pair_t pos;
  character_t *c;

  do {
    rand_pos(pos);
  } while (world.hiker_dist[pos[dim_y]][pos[dim_x]] == INT_MAX ||
           world.cur_map->cmap[pos[dim_y]][pos[dim_x]]         ||
           pos[dim_x] < 3 || pos[dim_x] > MAP_X - 4            ||
           pos[dim_y] < 3 || pos[dim_y] > MAP_Y - 4);

  world.cur_map->cmap[pos[dim_y]][pos[dim_x]] = c = (character_t *)malloc(sizeof (*c));
  c->npc = (npc_t *)malloc(sizeof (*c->npc));
  c->pos[dim_y] = pos[dim_y];
  c->pos[dim_x] = pos[dim_x];
  c->npc->ctype = char_hiker;
  c->npc->mtype = move_hiker;
  c->npc->dir[dim_x] = 0;
  c->npc->dir[dim_y] = 0;
  c->npc->defeated = 0;
  c->pc = NULL;
  c->symbol = 'h';
  c->next_turn = 0;
  heap_insert(&world.cur_map->turn, c);

  //  printf("Hiker at %d,%d\n", pos[dim_x], pos[dim_y]);
}

void new_rival()
{
  pair_t pos;
  character_t *c;

  do {
    rand_pos(pos);
  } while (world.rival_dist[pos[dim_y]][pos[dim_x]] == INT_MAX ||
           world.rival_dist[pos[dim_y]][pos[dim_x]] < 0        ||
           world.cur_map->cmap[pos[dim_y]][pos[dim_x]]         ||
           pos[dim_x] < 3 || pos[dim_x] > MAP_X - 4            ||
           pos[dim_y] < 3 || pos[dim_y] > MAP_Y - 4);

  world.cur_map->cmap[pos[dim_y]][pos[dim_x]] = c = (character_t *)malloc(sizeof (*c));
  c->npc = (npc_t *)malloc(sizeof (*c->npc));
  c->pos[dim_y] = pos[dim_y];
  c->pos[dim_x] = pos[dim_x];
  c->npc->ctype = char_rival;
  c->npc->mtype = move_rival;
  c->npc->dir[dim_x] = 0;
  c->npc->dir[dim_y] = 0;
  c->npc->defeated = 0;
  c->pc = NULL;
  c->symbol = 'r';
  c->next_turn = 0;
  heap_insert(&world.cur_map->turn, c);
}

void new_char_other()
{
  pair_t pos;
  character_t *c;

  do {
    rand_pos(pos);
  } while (world.rival_dist[pos[dim_y]][pos[dim_x]] == INT_MAX ||
           world.rival_dist[pos[dim_y]][pos[dim_x]] < 0        ||
           world.cur_map->cmap[pos[dim_y]][pos[dim_x]]         ||
           pos[dim_x] < 3 || pos[dim_x] > MAP_X - 4            ||
           pos[dim_y] < 3 || pos[dim_y] > MAP_Y - 4);

  world.cur_map->cmap[pos[dim_y]][pos[dim_x]] = c = (character_t *)malloc(sizeof (*c));
  c->npc = (npc_t *)malloc(sizeof (*c->npc));
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
  c->npc->defeated = 0;
  c->pc = NULL;
  c->next_turn = 0;
  heap_insert(&world.cur_map->turn, c);
}

void place_characters()
{
  world.cur_map->num_trainers = 2;

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
  } while (++world.cur_map->num_trainers < MIN_TRAINERS ||
           ((rand() % 100) < ADD_TRAINER_PROB));
}

void init_pc()
{
  int x, y;

  do {
    x = rand() % (MAP_X - 2) + 1;
    y = rand() % (MAP_Y - 2) + 1;
  } while (world.cur_map->map[y][x] != ter_path);

  world.pc.pos[dim_x] = x;
  world.pc.pos[dim_y] = y;
  world.pc.symbol = '@';
  world.pc.pc = (pc_t *)malloc(sizeof (*world.pc.pc));
  world.pc.npc = NULL;

  world.cur_map->cmap[y][x] = &world.pc;
  world.pc.next_turn = 0;

  heap_insert(&world.cur_map->turn, &world.pc);
}

void place_pc()
{
  character_t *c;

  if (world.pc.pos[dim_x] == 1) {
    world.pc.pos[dim_x] = MAP_X - 2;
  } else if (world.pc.pos[dim_x] == MAP_X - 2) {
    world.pc.pos[dim_x] = 1;
  } else if (world.pc.pos[dim_y] == 1) {
    world.pc.pos[dim_y] = MAP_Y - 2;
  } else if (world.pc.pos[dim_y] == MAP_Y - 2) {
    world.pc.pos[dim_y] = 1;
  }

  world.cur_map->cmap[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = &world.pc;

  if ((c = (character_t *)heap_peek_min(&world.cur_map->turn))) {
    world.pc.next_turn = c->next_turn;
  } else {
    world.pc.next_turn = 0;
  }
}

// New map expects cur_idx to refer to the index to be generated.  If that
// map has already been generated then the only thing this does is set
// cur_map.
int new_map(int teleport)
{
  int d, p;
  int e, w, n, s;
  int x, y;
  
  if (world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x]]) {
    world.cur_map = world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x]];
    place_pc();

    return 0;
  }

  world.cur_map                                             =
    world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x]] =
    (map_t *)malloc(sizeof (*world.cur_map));

  smooth_height(world.cur_map);
  
  if (!world.cur_idx[dim_y]) {
    n = -1;
  } else if (world.world[world.cur_idx[dim_y] - 1][world.cur_idx[dim_x]]) {
    n = world.world[world.cur_idx[dim_y] - 1][world.cur_idx[dim_x]]->s;
  } else {
    n = 3 + rand() % (MAP_X - 6);
  }
  if (world.cur_idx[dim_y] == WORLD_SIZE - 1) {
    s = -1;
  } else if (world.world[world.cur_idx[dim_y] + 1][world.cur_idx[dim_x]]) {
    s = world.world[world.cur_idx[dim_y] + 1][world.cur_idx[dim_x]]->n;
  } else  {
    s = 3 + rand() % (MAP_X - 6);
  }
  if (!world.cur_idx[dim_x]) {
    w = -1;
  } else if (world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x] - 1]) {
    w = world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x] - 1]->e;
  } else {
    w = 3 + rand() % (MAP_Y - 6);
  }
  if (world.cur_idx[dim_x] == WORLD_SIZE - 1) {
    e = -1;
  } else if (world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x] + 1]) {
    e = world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x] + 1]->w;
  } else {
    e = 3 + rand() % (MAP_Y - 6);
  }
  
  map_terrain(world.cur_map, n, s, e, w);
     
  place_boulders(world.cur_map);
  place_trees(world.cur_map);
  build_paths(world.cur_map);
  d = (abs(world.cur_idx[dim_x] - (WORLD_SIZE / 2)) +
       abs(world.cur_idx[dim_y] - (WORLD_SIZE / 2)));
  p = d > 200 ? 5 : (50 - ((45 * d) / 200));
  //  printf("d=%d, p=%d\n", d, p);
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

  if ((world.cur_idx[dim_x] == WORLD_SIZE / 2) &&
      (world.cur_idx[dim_y] == WORLD_SIZE / 2)) {
    init_pc();
  } else {
    place_pc();
  }

  pathfind(world.cur_map);
  if (teleport) {
    do {
      world.cur_map->cmap[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = NULL;
      world.pc.pos[dim_x] = rand_range(1, MAP_X - 2);
      world.pc.pos[dim_y] = rand_range(1, MAP_Y - 2);
    } while (world.cur_map->cmap[world.pc.pos[dim_y]][world.pc.pos[dim_x]] ||
             (move_cost[char_pc][world.cur_map->map[world.pc.pos[dim_y]]
                                                   [world.pc.pos[dim_x]]] ==
              INT_MAX)                                                      ||
             world.rival_dist[world.pc.pos[dim_y]][world.pc.pos[dim_x]] < 0);
    world.cur_map->cmap[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = &world.pc;
    pathfind(world.cur_map);
  }
  
  place_characters();

  return 0;
}

/*
static void print_map()
{
  int x, y;
  int default_reached = 0;

  printf("\n\n\n");

  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      if (world.cur_map->cmap[y][x]) {
        putchar(world.cur_map->cmap[y][x]->symbol);
      } else {
        switch (world.cur_map->map[y][x]) {
        case ter_boulder:
        case ter_mountain:
          putchar('%');
          break;
        case ter_tree:
        case ter_forest:
          putchar('^');
          break;
        case ter_path:
          putchar('#');
          break;
        case ter_mart:
          putchar('M');
          break;
        case ter_center:
          putchar('C');
          break;
        case ter_grass:
          putchar(':');
          break;
        case ter_clearing:
          putchar('.');
          break;
        default:
          default_reached = 1;
          break;
        }
      }
    }
    putchar('\n');
  }

  if (default_reached) {
    fprintf(stderr, "Default reached in %s\n", __FUNCTION__);
  }
}
*/

// The world is global because of its size, so init_world is parameterless
void init_world()
{
  world.quit = 0;
  world.cur_idx[dim_x] = world.cur_idx[dim_y] = WORLD_SIZE / 2;
  new_map(0);
}

void delete_world()
{
  int x, y;

  //Only correct because current game never leaves the initial map
  //Need to iterate over all maps in 1.05+
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

void print_hiker_dist()
{
  int x, y;

  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      if (world.hiker_dist[y][x] == INT_MAX) {
        printf("   ");
      } else {
        printf(" %5d", world.hiker_dist[y][x]);
      }
    }
    printf("\n");
  }
}

void print_rival_dist()
{
  int x, y;

  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      if (world.rival_dist[y][x] == INT_MAX || world.rival_dist[y][x] < 0) {
        printf("   ");
      } else {
        printf(" %02d", world.rival_dist[y][x] % 100);
      }
    }
    printf("\n");
  }
}

void leave_map(pair_t d)
{
  if (d[dim_x] == 0) {
    world.cur_idx[dim_x]--;
  } else if (d[dim_y] == 0) {
    world.cur_idx[dim_y]--;
  } else if (d[dim_x] == MAP_X - 1) {
    world.cur_idx[dim_x]++;
  } else {
    world.cur_idx[dim_y]++;
  }
  new_map(0);
}

void game_loop()
{
  character_t *c;
  pair_t d;
  
  while (!world.quit) {
    c = (character_t *)heap_remove_min(&world.cur_map->turn);

    move_func[c->npc ? c->npc->mtype : move_pc](c, d);

    world.cur_map->cmap[c->pos[dim_y]][c->pos[dim_x]] = NULL;
    if (c->pc && (d[dim_x] == 0 || d[dim_x] == MAP_X - 1 ||
                  d[dim_y] == 0 || d[dim_y] == MAP_Y - 1)) {
      leave_map(d);
      d[dim_x] = c->pos[dim_x];
      d[dim_y] = c->pos[dim_y];
    }
    world.cur_map->cmap[d[dim_y]][d[dim_x]] = c;

    if (c->pc) {
      // Performance bug - pathfinding runs twice after generating a new map
      pathfind(world.cur_map);
    }

    c->next_turn += move_cost[c->npc ? c->npc->ctype : char_pc]
                             [world.cur_map->map[d[dim_y]][d[dim_x]]];

    c->pos[dim_y] = d[dim_y];
    c->pos[dim_x] = d[dim_x];

    heap_insert(&world.cur_map->turn, c);
  }
}

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

void parse_pokemon(string path) {
    ifstream file;
    string line = "";
    string int_string = "";
    int id, species_id, height, weight, base_exp, order, is_default, i;
    string name;
    vector<pokemon> pokes;
    
    path.append("/pokemon.csv");
    file.open(path);
    
    getline(file, line);
    line = "";
    
    while(getline(file, line)) {
        stringstream input_string(line);
        
        getline(input_string, int_string, ',');
        id = atoi(int_string.c_str());
        
        getline(input_string, name, ',');
        
        getline(input_string, int_string, ',');
        species_id = atoi(int_string.c_str());
        
        getline(input_string, int_string, ',');
        height = atoi(int_string.c_str());
        
        getline(input_string, int_string, ',');
        weight = atoi(int_string.c_str());
        
        getline(input_string, int_string, ',');
        base_exp = atoi(int_string.c_str());
        
        getline(input_string, int_string, ',');
        order = atoi(int_string.c_str());
        
        getline(input_string, int_string, ',');
        is_default = atoi(int_string.c_str());
        
        pokemon poke(id, name, species_id, height, weight, base_exp, order, is_default);
        pokes.push_back(poke);
        
        line = "";
    }
    
    for(i = 0; i < (int)pokes.size(); i++) {
        pokes[i].print_pokemon();
    }
}

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
        }else{
            cout << ",";
        }
        if(generation_id != -1) {
            cout << ", Generation ID: " << generation_id;
        }else{
            cout << ",";
        }
        if(type_id != -1) {
            cout << ", Type ID: " << type_id;
        }else{
            cout << ",";
        }
        if(power != -1) {
            cout << ", Power: " << power;
        }else{
            cout << ",";
        }
        if(pp != -1) {
            cout << ", PP: " << pp;
        }else{
            cout << ",";
        }
        if(accuracy != -1) {
            cout << ", Accuracy: " << accuracy;
        }else{
            cout << ",";
        }
        if(priority != -1) {
            cout << ", Priority: " << priority;
        }else{
            cout << ",";
        }
        if(target_id != -1) {
            cout << ", Target ID: " << target_id;
        }else{
            cout << ",";
        }
        if(damage_class_id != -1) {
            cout << ", Damage Class ID: " << damage_class_id;
        }else{
            cout << ",";
        }
        if(effect_id != -1) {
            cout << ", Effect ID: " << effect_id;
        }else{
            cout << ",";
        }
        if(effect_chance != -1) {
            cout << ", Effect Chance: " << effect_chance;
        }else{
            cout << ",";
        }
        if(contest_type_id != -1) {
            cout << ", Contest Type ID: " << contest_type_id;
        }else{
            cout << ",";
        }
        if(contest_effect_id != -1) {
            cout << ", Contest Effect ID: " << contest_effect_id;
        }else{
            cout << ",";
        }
        if(super_contest_effect_id != -1) {
            cout << ", Super Contest Effect ID: " << super_contest_effect_id << endl;
        }else{
            cout << "," << endl;
        }
        cout << endl;
    }
};

void parse_moves(string path) {
    ifstream file;
    string line = "";
    string int_string = "";
    int id, generation_id, type_id, power, pp, accuracy, priority, target_id, damage_class_id, effect_id, effect_chance, contest_type_id, contest_effect_id, super_contest_effect_id, i;
    string name;
    vector<moves> moves_vec;
    
    path.append("/moves.csv");
    file.open(path);

    getline(file, line);
    line = "";
    
    while (getline(file, line)) {
        stringstream input_string(line);

        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            id = atoi(int_string.c_str());
        }
        else {
            id = -1;
        }
            
        getline(input_string, name, ',');

        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            generation_id = atoi(int_string.c_str());
        }
        else {
            generation_id = -1;
        }

        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            type_id = atoi(int_string.c_str());
        }
        else {
            type_id = -1;
        }

        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            power = atoi(int_string.c_str());
        }
        else {
            power = -1;
        }
        
        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            pp = atoi(int_string.c_str());
        }
        else {
            pp = -1;
        }
        
        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            accuracy = atoi(int_string.c_str());
        }
        else {
            accuracy = -1;
        }
        
        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            priority = atoi(int_string.c_str());
        }
        else {
            priority = -1;
        }
        
        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            target_id = atoi(int_string.c_str());
        }
        else {
            target_id = -1;
        }

        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            damage_class_id = atoi(int_string.c_str());
        }
        else {
            damage_class_id = -1;
        }

        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            effect_id = atoi(int_string.c_str());
        }
        else {
            effect_id = -1;
        }

        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            effect_chance = atoi(int_string.c_str());
        }
        else {
            effect_chance = -1;
        }
        
        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            contest_type_id = atoi(int_string.c_str());
        }
        else {
            contest_type_id = -1;
        }
        
        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            contest_effect_id = atoi(int_string.c_str());
        }
        else {
            contest_effect_id = -1;
        }
        
        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            super_contest_effect_id = atoi(int_string.c_str());
        }
        else {
            super_contest_effect_id = -1;
        }

        moves move(id, name, generation_id, type_id, power, pp, accuracy, priority, target_id, damage_class_id, effect_id, effect_chance, contest_type_id,
            contest_effect_id, super_contest_effect_id);

        moves_vec.push_back(move);
        line = "";
    }
    
    for(i = 0; i < (int)moves_vec.size(); i++) {
        moves_vec[i].print_moves();
    }
}

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
        }else{
            cout << ",";
        }
        if(version_group_id != -1) {
            cout << ", Version Group ID: " << version_group_id;
        }else{
            cout << ",";
        }
        if(move_id != -1) {
            cout << ", Move ID: " << move_id;
        }else{
            cout << ",";
        }
        if(pokemon_move_method_id != -1) {
            cout << ", Pokemon Move Method ID: " << pokemon_move_method_id;
        }else{
            cout << ",";
        }
        if(level != -1) {
            cout << ", Level: " << level;
        }else{
            cout << ",";
        }
        if(order != -1) {
            cout << ", Order: " << order << endl;
        }else{
            cout << "," << endl;
        }
        cout << endl;
    }
};

void parse_pokemon_moves(string path) {
    ifstream file;
    string line = "";
    string int_string = "";
    int pokemon_id, version_group_id, move_id, pokemon_move_method_id, level, order, i;
    vector<pokemon_moves> pokemon_moves_vec;

    path.append("/pokemon_moves.csv");
    file.open(path);

    getline(file, line);
    line = "";

    while(getline(file, line)) {
        stringstream input_string(line);

        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            pokemon_id = atoi(int_string.c_str());
        }
        else {
            pokemon_id = -1;
        }

        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            version_group_id = atoi(int_string.c_str());
        }
        else {
            version_group_id = -1;
        }

        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            move_id = atoi(int_string.c_str());
        }
        else {
            move_id = -1;
        }
        
        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            pokemon_move_method_id = atoi(int_string.c_str());
        }
        else {
            pokemon_move_method_id = -1;
        }
        
        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            level = atoi(int_string.c_str());
        }
        else {
            level = -1;
        }
        
        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            order = atoi(int_string.c_str());
        }
        else {
            order = -1;
        }

        pokemon_moves poke_moves(pokemon_id, version_group_id, move_id, pokemon_move_method_id, level, order);
        pokemon_moves_vec.push_back(poke_moves);

        line = "";
    }

    for(i = 0; i < (int)pokemon_moves_vec.size(); i++) {
        pokemon_moves_vec[i].print_pokemon_moves();
    }
}

class pokemon_species {
public:
    int id, generation_id, evolves_from_species_id, evolution_chain_id, color_id, shape_id, habitat_id, gender_rate, capture_rate, base_happiness, is_baby, hatch_counter,
        has_gender_differences, growth_rate_id, forms_switchable, is_legendary, is_mythical, order, conquest_order;
    string name;
    
    pokemon_species(int id, string name, int generation_id, int evolves_from_species_id, int evolution_chain_id, int color_id, int shape_id, int habitat_id, int gender_rate,
        int capture_rate, int base_happiness, int is_baby, int hatch_counter, int has_gender_differences, int growth_rate_id, int forms_switchable, int is_legendary, int is_mythical,
        int order, int conquest_order) {
                this->id = id;
                this->name = name;
                this->generation_id = generation_id;
                this->evolves_from_species_id = evolves_from_species_id;
                this->evolution_chain_id = evolution_chain_id;
                this->color_id = color_id;
                this->shape_id = shape_id;
                this->habitat_id = habitat_id;
                this->gender_rate = gender_rate;
                this->capture_rate = capture_rate;
                this->base_happiness = base_happiness;
                this->is_baby = is_baby;
                this->hatch_counter = hatch_counter;
                this->has_gender_differences = has_gender_differences;
                this->growth_rate_id = growth_rate_id;
                this->forms_switchable = forms_switchable;
                this->is_legendary = is_legendary;
                this->is_mythical = is_mythical;
                this->order = order;
                this->conquest_order = conquest_order;
    }

    void print_pokemon_species() {
        cout << "Species ID: " << id << ", Species Name: " << name << ", Generation ID: " << generation_id;
        if(evolves_from_species_id != -1) {
            cout << ", Evolves From ID: " << evolves_from_species_id;
        }else{
            cout << ",";
        }
        if(evolution_chain_id != -1) {
            cout << ", Evoltuion Chain ID: " << evolution_chain_id;
        }else{
            cout << ",";
        }
        cout << ", Color ID: " << color_id << ", Shape ID: " << shape_id;
        if(habitat_id != -1) {
            cout << ", Habitat ID: " << habitat_id;
        }else{
            cout << ",";
        }
        if(gender_rate != -1) {
            cout << ", Gender Rate: " << gender_rate;
        }else{
            cout << ",";
        }
        cout << ", Capture Rate: " << capture_rate << ", Base Happiness: " << base_happiness << ", Is Baby?: " << is_baby << ", Hatch Counter: " << hatch_counter
        << ", Has Gender Differences?: " << has_gender_differences << ", Growth Rate ID: " << growth_rate_id << ", Form Switchable?: " << forms_switchable
        << ", Legendary?: " << is_legendary << ", Mythical?: " << is_mythical;
        if(order != -1) {
            cout << " pokemon_species order: " << order;
        }else{
            cout << ",";
        }
        if(conquest_order != -1) {
            cout << " pokemon_species conquest_order: " << conquest_order << endl;
        }else{
            cout << "," << endl;
        }
        cout << endl;
    }
};

void parse_poke_species(string path) {
    ifstream file;
    string line = "";
    string int_string = "";
    int id, generation_id, evolves_from_species_id, evolution_chain_id, color_id, shape_id, habitat_id, gender_rate, capture_rate, base_happiness, is_baby, hatch_counter,
        has_gender_differences, growth_rate_id, forms_switchable, is_legendary, is_mythical, order, conquest_order, i;
    string name;
    vector<pokemon_species> pokemon_species_vec;

    path.append("/pokemon_species.csv");
    file.open(path);

    getline(file, line);
    line = "";

    while (getline(file, line)) {

        stringstream input_string(line);

        getline(input_string, int_string, ',');
        id = atoi(int_string.c_str());
        
        getline(input_string, name, ',');

        getline(input_string, int_string, ',');
        generation_id = atoi(int_string.c_str());

        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            evolves_from_species_id = atoi(int_string.c_str());
        }
        else {
            evolves_from_species_id = -1;
        }
        
        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            evolution_chain_id = atoi(int_string.c_str());
        }
        else {
            evolution_chain_id = -1;
        }
        
        getline(input_string, int_string, ',');
        color_id = atoi(int_string.c_str());

        getline(input_string, int_string, ',');
        shape_id = atoi(int_string.c_str());

        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            habitat_id = atoi(int_string.c_str());
        }
        else {
            habitat_id = -1;
        }

        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            gender_rate = atoi(int_string.c_str());
        }
        else {
            gender_rate = -1;
        }

        getline(input_string, int_string, ',');
        capture_rate = atoi(int_string.c_str());

        getline(input_string, int_string, ',');
        base_happiness = atoi(int_string.c_str());

        getline(input_string, int_string, ',');
        is_baby = atoi(int_string.c_str());

        getline(input_string, int_string, ',');
        hatch_counter = atoi(int_string.c_str());

        getline(input_string, int_string, ',');
        has_gender_differences = atoi(int_string.c_str());

        getline(input_string, int_string, ',');
        growth_rate_id = atoi(int_string.c_str());

        getline(input_string, int_string, ',');
        forms_switchable = atoi(int_string.c_str());

        getline(input_string, int_string, ',');
        is_legendary = atoi(int_string.c_str());

        getline(input_string, int_string, ',');
        is_mythical = atoi(int_string.c_str());

        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            order = atoi(int_string.c_str());
        }
        else {
            order = -1;
        }

        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            conquest_order = atoi(int_string.c_str());
        }
        else {
            conquest_order = -1;
        }

        pokemon_species poke_species(id, name, generation_id, evolves_from_species_id, evolution_chain_id, color_id, shape_id, habitat_id, gender_rate, capture_rate, base_happiness,
                                     is_baby, hatch_counter, has_gender_differences, growth_rate_id, forms_switchable, is_legendary, is_mythical, order, conquest_order);
        
        pokemon_species_vec.push_back(poke_species);
        line = "";
    }
    
    for(i = 0; i < (int)pokemon_species_vec.size(); i++) {
        pokemon_species_vec[i].print_pokemon_species();
    }
}

class experience {
public:
    int growth_rate, level, exp;
    
    experience(int growth_rate, int level, int exp) {
        this->growth_rate = growth_rate;
        this->level = level;
        this->exp = exp;
    }
    
    void print_exp() {
        cout << "Growth rate ID: " << growth_rate << ", Level: " << level << ", Experience: " << exp << endl;
    }
};

void parse_experience(string path) {
    ifstream file;
    string line = "";
    string int_string = "";
    int growth_rate, level, exp, i;
    vector<experience> experiences;
    
    path.append("/experience.csv");
    file.open(path);
    
    getline(file, line);
    line = "";
    
    while(getline(file, line)) {
        stringstream input_string(line);
        
        getline(input_string, int_string, ',');
        growth_rate = atoi(int_string.c_str());
        
        getline(input_string, int_string, ',');
        level = atoi(int_string.c_str());
        
        getline(input_string, int_string, ',');
        exp = atoi(int_string.c_str());
        
        experience exps(growth_rate, level, exp);
        experiences.push_back(exps);
        
        line = "";
    }
    
    for(i = 0; i < (int)experiences.size(); i++) {
        experiences[i].print_exp();
    }
}

class type_names {
public:
    int type_id, language_id;
    string name;
    
    type_names(int type_id, int language_id, string name) {
        this->type_id = type_id;
        this->language_id = language_id;
        this->name = name;
    }
    
    void print_type_names() {
        cout << "Type ID: " << type_id << ", Language ID: " << language_id << ", Name: " << name << endl;
    }
};

void parse_type_names(string path) {
    ifstream file;
    string line = "";
    string int_string = "";
    int type_id, language_id, i;
    string name;
    vector<type_names> type_names_vec;
    
    path.append("/type_names.csv");
    file.open(path);
    
    getline(file, line);
    line = "";
    
    while(getline(file, line)) {
        stringstream input_string(line);
        
        getline(input_string, int_string, ',');
        type_id = atoi(int_string.c_str());
        
        getline(input_string, int_string, ',');
        language_id = atoi(int_string.c_str());
        
        getline(input_string, name, ',');
        
        type_names types(type_id, language_id, name);
        type_names_vec.push_back(types);
        
        line = "";
    }
    
    for(i = 0; i < (int)type_names_vec.size(); i++) {
        type_names_vec[i].print_type_names();
    }
}

class pokemon_stats {
public:
    int pokemon_id, stat_id, base_stat, effort;
    //string name;
    
    pokemon_stats(int pokemon_id, int stat_id, int base_stat, int effort) {
        this->pokemon_id = pokemon_id;
        this->stat_id = stat_id;
        this->base_stat = base_stat;
        this->effort = effort;
    }
    
    void print_pokemon_stats() {
        cout << "Pokemon ID: " << pokemon_id << ", Stat ID: " << stat_id << ", Base Stat: " << base_stat << ", Effort: " << effort << endl;
    }
};

void parse_pokemon_stats(string path) {    
    ifstream file;
    string line = "";
    string int_string = "";
    int pokemon_id, stat_id, base_stat, effort;
    int i;
    vector<pokemon_stats> poke_stats_vec;
    
    path.append("/pokemon_stats.csv");
    file.open(path);

    getline(file, line);
    line = "";
    
    while(getline(file, line)) {
        stringstream input_string(line);
        
        getline(input_string, int_string, ',');
        pokemon_id = atoi(int_string.c_str());
        
        getline(input_string, int_string, ',');
        stat_id = atoi(int_string.c_str());
        
        getline(input_string, int_string, ',');
        base_stat = atoi(int_string.c_str());
        
        getline(input_string, int_string, ',');
        effort = atoi(int_string.c_str());
        
        pokemon_stats poke_stats(pokemon_id, stat_id, base_stat, effort);
        poke_stats_vec.push_back(poke_stats);
        
        line = "";
    }

    for(i = 0; i < (int)poke_stats_vec.size(); i++) {
        poke_stats_vec[i].print_pokemon_stats();
    }
}

class stats {
public:
    int id, damage_class_id, is_battle_only, game_index;
    string name;
    
    stats(int id, int damage_class_id, string name, int is_battle_only, int game_index) {
                this->id = id;
                this->damage_class_id = damage_class_id;
                this->name = name;
                this->is_battle_only = is_battle_only;
                this->game_index = game_index;
    }

    void print_stats() {
        //cout << "ID: " << id << ", Damage Class ID: " << damage_class_id << ", Identifier: " << name << ", Is Battle Only: " << is_battle_only << ", Game Index: " << game_index << endl;
        cout << "ID: " << id;

        if(damage_class_id != -1) {
            cout << ", Damage Class ID: " << damage_class_id;
        }else{
            cout << ",";
        }

        cout << ", Identifier: " << name << ", Is Battle Only: " << is_battle_only;

        if(game_index != -1) {
            cout << ", Game Index: " << game_index << endl;
        }else{
            cout << "," << endl;
        }
        cout << endl;
    }
};

void parse_stats(string path) {
    ifstream file;
    string line = "";
    string int_string = "";
    int id, damage_class_id, is_battle_only, game_index;
    int i;
    string name;
    vector<stats> stats_vec;

    path.append("/stats.csv");
    file.open(path);

    getline(file, line);
    line = "";

    while (getline(file, line)) {

        stringstream input_string(line);

        getline(input_string, int_string, ',');
        id = atoi(int_string.c_str());
        
        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            damage_class_id = atoi(int_string.c_str());
        }
        else {
            damage_class_id = -1;
        }

        //getline(input_string, int_string, ',');
        //damage_class_id = atoi(int_string.c_str());

        getline(input_string, name, ',');

        getline(input_string, int_string, ',');
        is_battle_only = atoi(int_string.c_str());

        //getline(input_string, int_string, ',');
        //game_index = atoi(int_string.c_str());

        getline(input_string, int_string, ',');
        if(!int_string.empty()) {
            game_index = atoi(int_string.c_str());
        }
        else {
            game_index = -1;
        }

        stats stats(id, damage_class_id, name, is_battle_only, game_index);
        
        stats_vec.push_back(stats);
        line = "";
    }
    
    for(i = 0; i < (int)stats_vec.size(); i++) {
        stats_vec[i].print_stats();
    }
}

class pokemon_types {
public:
    int pokemon_id, type_id, slot;
    
    pokemon_types(int pokemon_id, int type_id, int slot) {
        this->pokemon_id = pokemon_id;
        this->type_id = type_id;
        this->slot = slot;
    }
    
    void print_pokemon_types() {
        cout << "Pokemon ID: " << pokemon_id << ", Type ID: " << type_id << ", Slot: " << slot << endl;
    }
};

void parse_pokemon_types(string path) {    
    ifstream file;
    string line = "";
    string int_string = "";
    int pokemon_id, type_id, slot;
    int i;
    vector<pokemon_types> poke_types_vec;
    
    path.append("/pokemon_types.csv");
    file.open(path);

    getline(file, line);
    line = "";
    
    while(getline(file, line)) {
        stringstream input_string(line);
        
        getline(input_string, int_string, ',');
        pokemon_id = atoi(int_string.c_str());
        
        getline(input_string, int_string, ',');
        type_id = atoi(int_string.c_str());
        
        getline(input_string, int_string, ',');
        slot = atoi(int_string.c_str());
                
        pokemon_types poke_types(pokemon_id, type_id, slot);
        poke_types_vec.push_back(poke_types);
        
        line = "";
    }

    for(i = 0; i < (int)poke_types_vec.size(); i++) {
        poke_types_vec[i].print_pokemon_types();
    }
}


int main(int argc, char *argv[])
{
    string path;
    ifstream file;
    int worked = 0;
    
    if(argc > 1) {
        if(strcmp(argv[1], "pokemon") == 0) {
            path = "/share/cs327/pokedex/pokedex/data/csv";
            file.open(path);
            if(file.is_open()) {
                parse_pokemon(path);
                worked = 1;
            }
            path = "share/cs327/pokedex/pokedex/data/csv";
            file.open(path);
            if(file.is_open()) {
                parse_pokemon(path);
                worked = 1;
            }
            path = "";
            path.append(getenv("HOME"));
            path.append("/.poke327/pokedex/pokedex/data/csv");
            file.open(path);
            if(file.is_open()) {
                parse_pokemon(path);
                worked = 1;
            }
        }
        else if(strcmp(argv[1], "moves") == 0) {
            path = "/share/cs327/pokedex/pokedex/data/csv";
            file.open(path);
            if(file.is_open()) {
                parse_moves(path);
                worked = 1;
            }
            path = "share/cs327/pokedex/pokedex/data/csv";
            file.open(path);
            if(file.is_open()) {
                parse_moves(path);
                worked = 1;
            }
            path = "";
            path.append(getenv("HOME"));
            path.append("/.poke327/pokedex/pokedex/data/csv");
            file.open(path);
            if(file.is_open()) {
                parse_moves(path);
                worked = 1;
            }
        }
        else if(strcmp(argv[1], "pokemon_moves") == 0) {
            path = "/share/cs327/pokedex/pokedex/data/csv";
            file.open(path);
            if(file.is_open()) {
                parse_pokemon_moves(path);
                worked = 1;
            }
            path = "share/cs327/pokedex/pokedex/data/csv";
            file.open(path);
            if(file.is_open()) {
                parse_pokemon_moves(path);
                worked = 1;
            }
            path = "";
            path.append(getenv("HOME"));
            path.append("/.poke327/pokedex/pokedex/data/csv");
            file.open(path);
            if(file.is_open()) {
                parse_pokemon_moves(path);
                worked = 1;
            }
        }
        else if(strcmp(argv[1], "pokemon_species") == 0) {
            path = "/share/cs327/pokedex/pokedex/data/csv";
            file.open(path);
            if(file.is_open()) {
                parse_poke_species(path);
                worked = 1;
            }
            path = "share/cs327/pokedex/pokedex/data/csv";
            file.open(path);
            if(file.is_open()) {
                parse_poke_species(path);
                worked = 1;
            }
            path = "";
            path.append(getenv("HOME"));
            path.append("/.poke327/pokedex/pokedex/data/csv");
            file.open(path);
            if(file.is_open()) {
                parse_poke_species(path);
                worked = 1;
            }
        }
        else if(strcmp(argv[1], "experience") == 0) {
            path = "/share/cs327/pokedex/pokedex/data/csv";
            file.open(path);
            if(file.is_open()) {
                parse_experience(path);
                worked = 1;
            }
            path = "share/cs327/pokedex/pokedex/data/csv";
            file.open(path);
            if(file.is_open()) {
                parse_experience(path);
                worked = 1;
            }
            path = "";
            path.append(getenv("HOME"));
            path.append("/.poke327/pokedex/pokedex/data/csv");
            file.open(path);
            if(file.is_open()) {
                parse_experience(path);
                worked = 1;
            }
        }
        else if(strcmp(argv[1], "type_names") == 0) {
            path = "/share/cs327/pokedex/pokedex/data/csv";
            file.open(path);
            if(file.is_open()) {
                parse_type_names(path);
                worked = 1;
            }
            path = "share/cs327/pokedex/pokedex/data/csv";
            file.open(path);
            if(file.is_open()) {
                parse_type_names(path);
                worked = 1;
            }
            path = "";
            path.append(getenv("HOME"));
            path.append("/.poke327/pokedex/pokedex/data/csv");
            file.open(path);
            if(file.is_open()) {
                parse_type_names(path);
                worked = 1;
            }
        }
        else if(strcmp(argv[1], "pokemon_stats") == 0) {
            path = "/share/cs327/pokedex/pokedex/data/csv";
            file.open(path);
            if(file.is_open()) {
                parse_pokemon_stats(path);
                worked = 1;
            }
            path = "share/cs327/pokedex/pokedex/data/csv";
            file.open(path);
            if(file.is_open()) {
                parse_pokemon_stats(path);
                worked = 1;
            }
            path = "";
            path.append(getenv("HOME"));
            path.append("/.poke327/pokedex/pokedex/data/csv");
            file.open(path);
            if(file.is_open()) {
                parse_pokemon_stats(path);
                worked = 1;
            }
        }
        else if(strcmp(argv[1], "stats") == 0) {
            path = "/share/cs327/pokedex/pokedex/data/csv";
            file.open(path);
            if(file.is_open()) {
                parse_stats(path);
                worked = 1;
            }
            path = "share/cs327/pokedex/pokedex/data/csv";
            file.open(path);
            if(file.is_open()) {
                parse_stats(path);
                worked = 1;
            }
            path = "";
            path.append(getenv("HOME"));
            path.append("/.poke327/pokedex/pokedex/data/csv");
            file.open(path);
            if(file.is_open()) {
                parse_stats(path);
                worked = 1;
            }
        }
        else if(strcmp(argv[1], "pokemon_types") == 0) {
            path = "/share/cs327/pokedex/pokedex/data/csv";
            file.open(path);
            if(file.is_open()) {
                parse_pokemon_types(path);
                worked = 1;
            }
            path = "share/cs327/pokedex/pokedex/data/csv";
            file.open(path);
            if(file.is_open()) {
                parse_pokemon_types(path);
                worked = 1;
            }
            path = "";
            path.append(getenv("HOME"));
            path.append("/.poke327/pokedex/pokedex/data/csv");
            file.open(path);
            if(file.is_open()) {
                parse_pokemon_types(path);
                worked = 1;
            }
        }
        else {
            cout << "Incorrect file name." << endl;
        }
        if(worked == 0) {
            cout << "Did not get to the database." << endl;
        }
    }
    else {
        cout << "No file chosen to parse." << endl;
    }
    
    return 0;
    
  struct timeval tv;
  uint32_t seed;
  //  char c;
  //  int x, y;

  if (argc == 2) {
    seed = atoi(argv[1]);
  } else {
    gettimeofday(&tv, NULL);
    seed = (tv.tv_usec ^ (tv.tv_sec << 20)) & 0xffffffff;
  }

  printf("Using seed: %u\n", seed);
  srand(seed);

  io_init_terminal();
  
  init_world();

  /* print_hiker_dist(); */
  
  /*
  do {
    print_map();  
    printf("Current position is %d%cx%d%c (%d,%d).  "
           "Enter command: ",
           abs(world.cur_idx[dim_x] - (WORLD_SIZE / 2)),
           world.cur_idx[dim_x] - (WORLD_SIZE / 2) >= 0 ? 'E' : 'W',
           abs(world.cur_idx[dim_y] - (WORLD_SIZE / 2)),
           world.cur_idx[dim_y] - (WORLD_SIZE / 2) <= 0 ? 'N' : 'S',
           world.cur_idx[dim_x] - (WORLD_SIZE / 2),
           world.cur_idx[dim_y] - (WORLD_SIZE / 2));
    scanf(" %c", &c);
    switch (c) {
    case 'n':
      if (world.cur_idx[dim_y]) {
        world.cur_idx[dim_y]--;
        new_map();
      }
      break;
    case 's':
      if (world.cur_idx[dim_y] < WORLD_SIZE - 1) {
        world.cur_idx[dim_y]++;
        new_map();
      }
      break;
    case 'e':
      if (world.cur_idx[dim_x] < WORLD_SIZE - 1) {
        world.cur_idx[dim_x]++;
        new_map();
      }
      break;
    case 'w':
      if (world.cur_idx[dim_x]) {
        world.cur_idx[dim_x]--;
        new_map();
      }
      break;
     case 'q':
      break;
    case 'f':
      scanf(" %d %d", &x, &y);
      if (x >= -(WORLD_SIZE / 2) && x <= WORLD_SIZE / 2 &&
          y >= -(WORLD_SIZE / 2) && y <= WORLD_SIZE / 2) {
        world.cur_idx[dim_x] = x + (WORLD_SIZE / 2);
        world.cur_idx[dim_y] = y + (WORLD_SIZE / 2);
        new_map();
      }
      break;
    case '?':
    case 'h':
      printf("Move with 'e'ast, 'w'est, 'n'orth, 's'outh or 'f'ly x y.\n"
             "Quit with 'q'.  '?' and 'h' print this help message.\n");
      break;
    default:
      fprintf(stderr, "%c: Invalid input.  Enter '?' for help.\n", c);
      break;
    }
  } while (c != 'q');

  */

  game_loop();
  
  delete_world();

  io_reset_terminal();
  
  return 0;
}