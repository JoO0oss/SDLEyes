//http://lazyfoo.net/tutorials/SDL/01_hello_SDL/index2.php
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include "circle.h"

const int INIT_SCREEN_WIDTH = 640;
const int INIT_SCREEN_HEIGHT = 480;

const int TPS = 60;
const int END_ANIMATION_DURATION = 300;

const SDL_Colour BACKGROUND_COLOUR = {0, 0, 0};
const SDL_Colour WHITES_COLOUR = {255, 255, 255};
const SDL_Colour PUPILS_COLOUR = {0, 0, 0};

const int GOOGLE_MODE = 1; // The way which the googly eyes "google" around. 0 essentially reflects the mouse's absolute position on the screen, 1 has the pupils directed towards the pointer.

// If width expands above this, then screen scale goes by height, if height expands above this, then screen scale goes by width.
// It prevents excessively long or wide windows throwing the eyes out of view.
const float IDEAL_RATIO = 1.6;
const float WHITES_RADIUS = 0.14;
const float PUPIL_RADIUS = 0.03;

const float EYE_1X = 0.3;
const float EYE_1Y = 0.5;
const float EYE_2X = 0.7;
const float EYE_2Y = 0.5;

// Use this structure to store normalised coordinates, everything else can just use inidvidual variables.
struct ncoord
{
    double x;
    double y;
};

// Where wx, wy is the centre of the window, mx, my is the mouse position and sx, sy is the screen edge:
// L1, the line between the eyes and the mouse: y - wy = m (x - wx),  m = wy-my/wx-mx
// L2, the line making up the edge of the screen is 1 of 4 things:
//  x = 0 (left edge)
//  x = 1 (right edge)
//  y = 0 (top edge)
//  y = 1 (bottom edge)
// Once you have determined which of those is L2, you can solve a simultaneous equation to find the intersection, (sx, sy).

// Extrapolate the position where the line between the centre and the mouse meets the screen.
void linear_extrapolate(struct ncoord n_centre, struct ncoord n_mouse, struct ncoord * n_screen_edge)
{
    double eyes_to_mouse_gradient = (n_mouse.y - n_centre.y) / (n_mouse.x - n_centre.x);

    if(n_centre.x <= n_mouse.x && n_centre.y <= n_mouse.y)
    {
        // L1 must therefore intersect with either x = 1 or y = 1.

        // Assume L2 is y = 1, if you find that solving x gives x > 1, then you know x = 1 is the screen edge.
        n_screen_edge->y = 1;
        n_screen_edge->x = (1 - n_centre.y) / eyes_to_mouse_gradient + n_centre.x;
        // L1: y - wy = m (x - wx), L2: y = 1 solve simultaneously through  1 - mx = m (x - wx) , therefore  x = (1 - mx) / m + wx .

        if(n_screen_edge->x > 1)
        {
            n_screen_edge->x = 1;
            n_screen_edge->y = eyes_to_mouse_gradient * (1 - n_centre.x) + n_centre.y;
        }
    } else if (n_centre.x <= n_mouse.x && n_centre.y >= n_mouse.y) {
        // L1 must therefore intersect with either x = 1 or y = 0.

        // Assume L2 is y = 0.
        n_screen_edge->y = 0;
        n_screen_edge->x = n_centre.x - n_centre.y / eyes_to_mouse_gradient;

        if(n_screen_edge->x > 1)
        {
            n_screen_edge->x = 1;
            n_screen_edge->y = eyes_to_mouse_gradient * (1 - n_centre.x) + n_centre.y;
        }
    } else if (n_centre.x >= n_mouse.x && n_centre.y <= n_mouse.y) {
        // L1 must therefore intersect with either x = 0 or y = 1.

        // Assume L2 is y = 1.
        n_screen_edge->y = 1;
        n_screen_edge->x = (1 - n_centre.y) / eyes_to_mouse_gradient + n_centre.x;

        if(n_screen_edge->x < 0)
        {
            n_screen_edge->x = 0;
            n_screen_edge->y = n_centre.y - eyes_to_mouse_gradient * n_centre.x;
        }
    } else if (n_centre.x >= n_mouse.x && n_centre.y >= n_mouse.y) {
        // L1 must therefore intersect with either x = 0 or y = 0.

        // Assume L2 is y = 0.
        n_screen_edge->y = 0;
        n_screen_edge->x = n_centre.x - n_centre.y / eyes_to_mouse_gradient;

        if(n_screen_edge->x < 0)
        {
            n_screen_edge->x = 0;
            n_screen_edge->y = n_centre.y - eyes_to_mouse_gradient * n_centre.x;
        }
    }
}

int main(int argc, char* args[])
{
    printf("Hello!\n");

    bool run = true;

    int screen_width = 0; // Absolute width and height of the monitor/screen, in pixels.
    int screen_height = 0;
    int window_x = 0;
    int window_y = 0;
    int window_width = INIT_SCREEN_WIDTH;
    int window_height = INIT_SCREEN_HEIGHT;
    int mouse_x = 0;
    int mouse_y = 0;
    double window_centre_x = 0; // Absolute coordinates of the normalised numbers below.
    double window_centre_y = 0;
    double screen_edge_x = 0;
    double screen_edge_y = 0;

    // n stands for normalised.
    /*
    double n_window_centre_x = 0.0; // Centre of the window as a proportion of the whole screen.
    double n_window_centre_y = 0.0;
    double n_mouse_x = 0.0; // Mouse position as a proportion of the whole screen.
    double n_mouse_y = 0.0;
    double n_screen_edge_x = 0.0; // Position on the edge of the screen where the eyes are looking towards.
    double n_screen_edge_y = 0.0;
    */

    struct ncoord n_eye_centre = {0.0, 0.0}; // Centre of the window as a proportion of the whole screen.
    struct ncoord n_mouse = {0.0, 0.0}; // Mouse position as a proportion of the whole screen.
    struct ncoord n_screen_edge = {0.0, 0.0}; // Position on the edge of the screen where the eyes are looking towards.

    double mouse_distance = 0.0; // Distance betwen eyes and mouse.
    double edge_distance = 0.0; // Distance between eyes and screen edge.

    // eye_x and eye_y are the eye centres, relative to the window.
    double eye_x = 0.0;
    double eye_y = 0.0;
    double a_eye_x = 0.0; // Absolute eye centre coordinates.
    double a_eye_y = 0.0;

    // pupil1_x and pupil1_y are the pupil centres, relative to their respective eye centres. These are kept in separate variables since these are the end products of the calculations and cannot be done on a per-eye basis.
    int pupil1_x = 0;
    int pupil1_y = 0;
    int pupil2_x = 0;
    int pupil2_y = 0;

    // This is the distance between the centre of each eye and the mouse as a proportion of the eye radius. Used as part of normalising the delta x and y between eye and mouse.
    double mouse_distance_r = 0.0; // Distance between eyes and mouse compared to the eye radius.

    double distance_modifier = 0.0; // Distance modifier to set how far the pupils are from the centre of the eye.

    double window_scale = 0.0;

    int ms_per_frame = 1000 / TPS;

    // Initialise SDL2.

    SDL_Window * window = NULL;
    SDL_Renderer * renderer = NULL;
    
    if(SDL_Init(SDL_INIT_EVERYTHING))
    {
        printf("Error initialising SDL: %s\n", SDL_GetError());
    }
    
    window = SDL_CreateWindow("Eyes", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, INIT_SCREEN_WIDTH, INIT_SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if(window == NULL)
    {
        printf("Error creating window: %s\n", SDL_GetError());
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(renderer == NULL)
    {
        printf("Error creating renderer: %s\n", SDL_GetError());
    }
    
    SDL_DisplayMode display_mode;
    SDL_GetDesktopDisplayMode(0, &display_mode);
    screen_width = display_mode.w;
    screen_height = display_mode.h;

    // Initialisation finished.

    SDL_Event event;
    while(run)
    {
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
            {
                run = false;
            }
        }
        // Get the mouse position, even when the cursor is outside the window.
        SDL_GetGlobalMouseState(&mouse_x, &mouse_y);
        SDL_GetWindowPosition(window, &window_x, &window_y);
        SDL_GetWindowSize(window, &window_width, &window_height);

        if(window_width / (double) window_height > IDEAL_RATIO)
        {
            // If window_width has become excessively wide, use window_height to determine the scale.
            window_scale = (double) window_height * IDEAL_RATIO;
        }
        else
        {
            // If window_height has become excessively tall, use window_width to determine the scale.
            window_scale = (double) window_width;
        }

        if(GOOGLE_MODE == 0)
        {
            eye_x = round(EYE_1X * window_width);
            eye_y = round(EYE_1Y * window_height);
            // Just use a simple linear function of the proportion of the mouse across the screen.
            // 1.1 is an arbitrary number, it may want to change for different sizes of pupils.
            pupil1_x = round((mouse_x / (double) screen_width - 0.5) * (WHITES_RADIUS) * window_scale * 1.1) + (int) eye_x;
            pupil1_y = round((mouse_y / (double) screen_height - 0.5) * (WHITES_RADIUS) * window_scale * 1.1) + (int) eye_y;

            eye_x = round(EYE_2X * window_width);
            eye_y = round(EYE_2Y * window_height);    
            pupil2_x = round((mouse_x / (double) screen_width - 0.5) * (WHITES_RADIUS) * window_scale * 1.1) + (int) eye_x;
            pupil2_y = round((mouse_y / (double) screen_height - 0.5) * (WHITES_RADIUS) * window_scale * 1.1) + (int) eye_y;
        } else {
            // This google mode finds the distance of the mouse to the eye as a proportion of the distance to the edge of the screen in the same direction. That proportion is used as the pupil distance as a proportion of the eye radius.

            // EYE 1 DONE HERE.
            eye_x = round(EYE_1X * window_width);
            eye_y = round(EYE_1Y * window_height);


            // Remember a means absolute (0 - 1920x0 - 1280), n means normalised (0 - 1 in both axes).
            a_eye_x = eye_x + window_x;
            a_eye_y = eye_y + window_y;

            n_eye_centre.x = a_eye_x / screen_width;
            n_eye_centre.y = a_eye_y / screen_height;
            n_mouse.x = mouse_x / (double) screen_width;
            n_mouse.y = mouse_y / (double) screen_height;

            linear_extrapolate(n_eye_centre, n_mouse, &n_screen_edge);

            screen_edge_x = n_screen_edge.x * screen_width;
            screen_edge_y = n_screen_edge.y * screen_height;

            // Now find the distance from the eyes to the screen edge.
            mouse_distance = sqrt(pow(a_eye_x - mouse_x, 2) + pow(a_eye_y - mouse_y, 2));
            edge_distance = sqrt(pow(a_eye_x - screen_edge_x, 2) + pow(a_eye_y - screen_edge_y, 2));

            mouse_distance_r = sqrt((mouse_x - a_eye_x) * (mouse_x - a_eye_x) + (mouse_y - a_eye_y) * (mouse_y - a_eye_y));

            // This is the proportional distance of the eyes to the mouse compared to the eyes traced toward the edge of the screen.
            distance_modifier = mouse_distance_r / edge_distance;

            // Get the positions of pupils relative to the eyes.
            pupil1_x = round(((double) mouse_x - a_eye_x) / mouse_distance_r * (WHITES_RADIUS - PUPIL_RADIUS) * window_scale * distance_modifier) + eye_x;
            pupil1_y = round(((double) mouse_y - a_eye_y) / mouse_distance_r * (WHITES_RADIUS - PUPIL_RADIUS) * window_scale * distance_modifier) + eye_y;


            // EYE 2 DONE HERE.
            eye_x = round(EYE_2X * window_width);
            eye_y = round(EYE_2Y * window_height);

            a_eye_x = eye_x + window_x;
            a_eye_y = eye_y + window_y;

            n_eye_centre.x = a_eye_x / screen_width;
            n_eye_centre.y = a_eye_y / screen_height;
            n_mouse.x = mouse_x / (double) screen_width;
            n_mouse.y = mouse_y / (double) screen_height;

            linear_extrapolate(n_eye_centre, n_mouse, &n_screen_edge);

            screen_edge_x = n_screen_edge.x * screen_width;
            screen_edge_y = n_screen_edge.y * screen_height;

            // Now find the distance from the eyes to the screen edge.
            mouse_distance = sqrt(pow(a_eye_x - mouse_x, 2) + pow(a_eye_y - mouse_y, 2));
            edge_distance = sqrt(pow(a_eye_x - screen_edge_x, 2) + pow(a_eye_y - screen_edge_y, 2));

            mouse_distance_r = sqrt((mouse_x - a_eye_x) * (mouse_x - a_eye_x) + (mouse_y - a_eye_y) * (mouse_y - a_eye_y));

            // This is the proportional distance of the eyes to the mouse compared to the eyes traced toward the edge of the screen.
            distance_modifier = mouse_distance_r / edge_distance;

            // Get the positions of pupils relative to the eyes.
            pupil2_x = round(((double) mouse_x - a_eye_x) / mouse_distance_r * (WHITES_RADIUS - PUPIL_RADIUS) * window_scale * distance_modifier) + eye_x;
            pupil2_y = round(((double) mouse_y - a_eye_y) / mouse_distance_r * (WHITES_RADIUS - PUPIL_RADIUS) * window_scale * distance_modifier) + eye_y;
        }

        // Clear the screen first.

        SDL_SetRenderDrawColor(renderer, BACKGROUND_COLOUR.r, BACKGROUND_COLOUR.g, BACKGROUND_COLOUR.b, 255);
        SDL_RenderClear(renderer);

        // Draw the white circles
        SDL_SetRenderDrawColor(renderer, WHITES_COLOUR.r, WHITES_COLOUR.g, WHITES_COLOUR.b, 255);
        draw_circle(renderer, round(EYE_1X * window_width), round(EYE_1Y * window_height), round(window_scale * WHITES_RADIUS));
        draw_circle(renderer, round(EYE_2X * window_width), round(EYE_2Y * window_height), round(window_scale * WHITES_RADIUS));

        // Draw the pupils.
        SDL_SetRenderDrawColor(renderer, PUPILS_COLOUR.r, PUPILS_COLOUR.g, PUPILS_COLOUR.b, 255);
        draw_circle(renderer, pupil1_x, pupil1_y, round(window_scale * PUPIL_RADIUS));
        draw_circle(renderer, pupil2_x, pupil2_y, round(window_scale * PUPIL_RADIUS));
    
        // Update the screen.
        SDL_RenderPresent(renderer);
        SDL_Delay(ms_per_frame);
    }

    // Closing animation.
    SDL_SetRenderDrawColor(renderer, BACKGROUND_COLOUR.r, BACKGROUND_COLOUR.g, BACKGROUND_COLOUR.b, 255);

    double max_progress = END_ANIMATION_DURATION / ms_per_frame;
    for(int progress = 0; progress <= max_progress + 1; progress++)
    {
        //progress = (double) progress_ms / END_ANIMATION_DURATION;
        draw_circle(renderer, round(EYE_1X * window_width), round((EYE_1Y - 2 * WHITES_RADIUS * (1 - progress / max_progress)) * window_height), round(window_scale * WHITES_RADIUS));
        draw_circle(renderer, round(EYE_2X * window_width), round((EYE_2Y - 2 * WHITES_RADIUS * (1 - progress / max_progress)) * window_height), round(window_scale * WHITES_RADIUS));

        SDL_RenderPresent(renderer);
        SDL_Delay(ms_per_frame);
    }

    SDL_Delay(250);

    // Uninitialise everything.
    
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    printf("Goodbye!\n");
    return 0;
}
