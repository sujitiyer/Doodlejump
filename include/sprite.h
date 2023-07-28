#ifndef __SPRITE_H__
#define __SPRITE_H__

#include "body.h"
#include "color.h"
#include "list.h"
#include "polygon.h"
#include "scene.h"
#include "sdl_wrapper.h"
#include "state.h"
#include "vector.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// body type identifiers
typedef enum {
  PLAYER,
  PLATFORM,
  BULLET,
  SPRING,
  MONSTER,
  BLACKHOLE,
  JETPACK,
  PLAYER_WITH_JETPACK,
  MOVING_PLATFORM
} sprite_type_t;

/**
 * Generates the player sprite for the doodlejump game.
 *
 * @param center the center of the neck of the player
 * @return a pointer to the newly allocated body
 */
body_t *generate_player(vector_t center);

/**
 * Generates rectangles.
 *
 * @param pos position of the rectangle
 * @param dim dimension of the rectangle
 * @return a pointer to the newly allocated list of points for the rectangle
 */
list_t *generate_rect_shape(vector_t pos, vector_t dim);

/**
 * Generate a circle
 * 
 * @param center of circle
 * @param radius of circle
 * @return a pointer to the newly allocated list of points for the circle
*/
list_t *generate_circle_shape(vector_t center, size_t radius);

/**
 * Generates a platform
 *
 * @param pos position of the rectangle
 * @return a pointer to the newly allocated body of the platform
 */
body_t *generate_platform(vector_t pos);

/**
 * Generates a blue platform
 *
 * @param pos position of the rectangle
 * @return a pointer to the newly allocated body of the platform
 */
body_t *generate_blue_platform(vector_t pos);

/**
 * Generates the spring sprite for the doodlejump game.
 *
 * @param pos position of the rectangle
 * @return a pointer to the newly allocated body of a spring
 */
body_t *generate_spring(vector_t pos);

/**
 * Generates the jetpack sprite for the doodlejump game.
 *
 * @param pos position of the rectangle
 * @return a pointer to the newly allocated body of a jetpack
 */
body_t *generate_jetpack(vector_t pos);

/**
 * Generates a bullet for the doodlejump game.
 * 
 * @param center position of the bullet
 * @return a pointer to the newly  allocated body of a bullet
*/
body_t *generate_bullet(vector_t center);

/**
 * Generates a monster sprite for the doodlejump game.
 *
 * @param center the center of the nmonster
 * @return a pointer to the newly allocated body
 */
body_t *generate_monster(vector_t center);

/**
 * Generates a blackhole sprite for the doodlejump game.
 *
 * @param center the center of the nmonster
 * @return a pointer to the newly allocated body
 */
body_t *generate_blackhole(vector_t center);

#endif