#include "circle.h"
#include <math.h>

void draw_circle(SDL_Renderer * renderer, int x, int y, int r)
{
    if(r <= 1)
    {
        SDL_RenderDrawPoint(renderer, x, y);
    } else if (r == 2) {
        SDL_RenderDrawPoint(renderer, x, y);
        SDL_RenderDrawPoint(renderer, x + 1, y);
        SDL_RenderDrawPoint(renderer, x, y + 1);
        SDL_RenderDrawPoint(renderer, x - 1, y);
        SDL_RenderDrawPoint(renderer, x, y - 1);
    } else {

        int inside_rect_radius = round(r / sqrt(2)) - 1;
        int r2 = r * r;

        SDL_Rect inside_rect = {x - inside_rect_radius, y - inside_rect_radius, inside_rect_radius * 2 + 1, inside_rect_radius * 2 + 1};
        SDL_RenderFillRect(renderer, &inside_rect);

        int width_coord;
        SDL_Rect temp_line_rect = {0, 0, 0, 0};
        for(int depth = inside_rect_radius+1; depth <= r; depth++)
        {
            // Guarantees a straight line, since SDL_RenderDrawLine sometimes draws just slightly incorrectly.
            width_coord = round(sqrt(r2 - depth * depth));
            
            // On the first pass through, some numbers result in the 4 temp_line_rect positions overlapping by one pixel.
            if(width_coord > inside_rect_radius + 1)
            {
                // To fix that, just artificially make the line smaller by 1, then where 2 rectangles would have overlapped, just draw a single pixel.
                width_coord -= 1;
                SDL_RenderDrawPoint(renderer, x + width_coord, y + width_coord);
                SDL_RenderDrawPoint(renderer, x - width_coord, y + width_coord);
                SDL_RenderDrawPoint(renderer, x + width_coord, y - width_coord);
                SDL_RenderDrawPoint(renderer, x - width_coord, y - width_coord);
            }
            
            temp_line_rect.x = x - width_coord + 1;
            temp_line_rect.y = y - depth;
            temp_line_rect.w = width_coord * 2 - 1;
            temp_line_rect.h = 1;
            SDL_RenderFillRect(renderer, &temp_line_rect);

            temp_line_rect.y = y + depth;
            SDL_RenderFillRect(renderer, &temp_line_rect);

            temp_line_rect.x = x - depth;
            temp_line_rect.y = y - width_coord + 1;
            temp_line_rect.w = 1;
            temp_line_rect.h = width_coord * 2 - 1;
            SDL_RenderFillRect(renderer, &temp_line_rect);

            temp_line_rect.x = x + depth;
            SDL_RenderFillRect(renderer, &temp_line_rect);
        }
    }
}