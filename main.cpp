#include <SDL.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <random>
#include <ctime>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int INITIAL_LENGTH = 48;
const int APPLE_RADIUS = 8;
const int SEGMENTS_PER_APPLE = 48;
const int SPRITE_WIDTH = 32;
const double SEGMENT_RADIUS = 5.0;
const double SEGMENT_CLIPPING_RADIUS = 1.5;
const double MOVE_SPEED = 3.0; // Twice the clipping radius for best results
const double TURN_SPEED = 0.08;

class Circle
{
public:
    Circle(double x, double y, double radius, double clippingRadius)
    {
        m_x = x;
        m_y = y;
        m_r = radius;
        m_rc = clippingRadius;
    }
    double dxTo(Circle &c)
    {
        double dx = c.m_x - m_x;
        while(dx > SCREEN_WIDTH/2)
        {
            dx -= SCREEN_WIDTH;
        }
        while(dx < -SCREEN_WIDTH/2)
        {
            dx += SCREEN_WIDTH;
        }
        return dx;
    }
    double dyTo(Circle &c)
    {
        double dy = c.m_y - m_y;
        while(dy > SCREEN_HEIGHT/2)
        {
            dy -= SCREEN_HEIGHT;
        }
        while(dy < -SCREEN_HEIGHT/2)
        {
            dy += SCREEN_HEIGHT;
        }
        return dy;
    }
    double angleTo(Circle &c)
    {
        return atan2(dyTo(c), dxTo(c));
    }
    bool isTouching(Circle &c)
    {
        double dx = dxTo(c);
        double dy = dyTo(c);
        double d = c.m_r + m_r;
        return dx * dx + dy * dy <= d * d;
    }
    void translate(double angle, double distance)
    {
        m_x += distance * cos(angle);
        m_y += distance * sin(angle);
        if (m_x > SCREEN_WIDTH)
        {
            m_x -= SCREEN_WIDTH;
        }
        if (m_x < 0)
        {
            m_x += SCREEN_WIDTH;
        }
        if (m_y > SCREEN_HEIGHT)
        {
            m_y -= SCREEN_HEIGHT;
        }
        if (m_y < 0)
        {
            m_y += SCREEN_HEIGHT;
        }
    }
    void bringTowards(Circle &c, double distance)
    {
        double dx = dxTo(c);
        double dy = dyTo(c);
        double angle = atan2(dy, dx);
        translate(angle, distance);
    }
    void clipAgainst(Circle &c)
    {
        //Moves this circle to be tangent to the target circle
        double dx = dxTo(c);
        double dy = dyTo(c);
        translate(c.angleTo(*this), c.m_rc + m_rc - sqrt(dx * dx + dy * dy));
    }
    void setPosition(double x, double y)
    {
        m_x = x;
        m_y = y;
    }
    void render(SDL_Renderer* renderer, SDL_Texture* sprite, int r, int g, int b)
    {
        SDL_Rect src;
        src.x = 0;
        src.y = 0;
        src.w = SPRITE_WIDTH;
        src.h = SPRITE_WIDTH;
        SDL_Rect dst;
        dst.x = m_x - m_r;
        dst.y = m_y - m_r;
        dst.w = m_r * 2;
        dst.h = m_r * 2;
        SDL_SetTextureColorMod(sprite, r, g, b);
        SDL_RenderCopy(renderer, sprite, &src, &dst);
        //Screen wrapping
        bool onRightBorder = m_x >= SCREEN_WIDTH - m_r;
        bool onLeftBorder = m_x <= m_r;
        bool onTopBorder = m_y <= m_r;
        bool onBottomBorder = m_y >= SCREEN_HEIGHT - m_r;
        if (onRightBorder)
        {
            dst.x -= SCREEN_WIDTH;
            SDL_RenderCopy(renderer, sprite, &src, &dst);
            if (onTopBorder)
            {
                dst.y += SCREEN_HEIGHT;
                SDL_RenderCopy(renderer, sprite, &src, &dst);
                dst.y -= SCREEN_HEIGHT;
            }
            if (onBottomBorder)
            {
                dst.y -= SCREEN_HEIGHT;
                SDL_RenderCopy(renderer, sprite, &src, &dst);
                dst.y += SCREEN_HEIGHT;
            }
            dst.x += SCREEN_WIDTH;
        }
        if (onLeftBorder)
        {
            dst.x += SCREEN_WIDTH;
            SDL_RenderCopy(renderer, sprite, &src, &dst);
            if (onTopBorder)
            {
                dst.y += SCREEN_HEIGHT;
                SDL_RenderCopy(renderer, sprite, &src, &dst);
                dst.y -= SCREEN_HEIGHT;
            }
            if (onBottomBorder)
            {
                dst.y -= SCREEN_HEIGHT;
                SDL_RenderCopy(renderer, sprite, &src, &dst);
                dst.y += SCREEN_HEIGHT;
            }
            dst.x -= SCREEN_WIDTH;
        }
        if (onTopBorder)
        {
            dst.y += SCREEN_HEIGHT;
            SDL_RenderCopy(renderer, sprite, &src, &dst);
            dst.y -= SCREEN_HEIGHT;
        }
        if (onBottomBorder)
        {
            dst.y -= SCREEN_HEIGHT;
            SDL_RenderCopy(renderer, sprite, &src, &dst);
            dst.y += SCREEN_HEIGHT;
        }
    }
private:
    double m_x;
    double m_y;
    int m_r;
    int m_rc;
};

int main(int argc, char* arcv[])
{
    //Initialization
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Snake", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Surface* temp = SDL_LoadBMP("circle.bmp");
    SDL_SetColorKey(temp, SDL_TRUE, 0);
    SDL_Texture* circleSprite = SDL_CreateTextureFromSurface(renderer, temp);
    SDL_FreeSurface(temp);
    std::vector<Circle> snake = {};
    for(int i = 0; i<INITIAL_LENGTH; i++)
    {
        snake.push_back(Circle(SCREEN_WIDTH / 2 - SEGMENT_CLIPPING_RADIUS * 2 * i, SCREEN_HEIGHT / 2, SEGMENT_RADIUS, SEGMENT_CLIPPING_RADIUS));
    }
    Circle apple(rand() * SCREEN_WIDTH / RAND_MAX, rand() * SCREEN_HEIGHT / RAND_MAX, APPLE_RADIUS, APPLE_RADIUS);
    double angle = 0;
    int currentLength = INITIAL_LENGTH;
    SDL_Event event;
    bool leftPressed = false;
    bool rightPressed = false;
    int score = 0;
    bool looping = true;

    //Game loop
    while(looping)
    {
        int frameStart = SDL_GetTicks();
        //Process input
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT)
            {
                looping = false;
            }
            else if(event.type == SDL_KEYDOWN)
            {
                switch(event.key.keysym.sym)
                {
                    case SDLK_LEFT:
                    leftPressed = true;
                    break;
                    case SDLK_RIGHT:
                    rightPressed = true;
                    break;
                }
            }
            else if(event.type == SDL_KEYUP)
            {
                switch(event.key.keysym.sym)
                {
                    case SDLK_LEFT:
                    leftPressed = false;
                    break;
                    case SDLK_RIGHT:
                    rightPressed = false;
                    break;
                }
            }
        }

        //Game logic
        for(int i = currentLength - 1; i > 0; i--)
        {
            snake[i].bringTowards(snake[i-1], MOVE_SPEED);
        }
        snake[0].translate(angle, MOVE_SPEED);
        for(int i = 1; i < currentLength; i++)
        {
            snake[i].clipAgainst(snake[i-1]);
        }
        if(snake[0].isTouching(apple))
        {
            apple.setPosition(rand() * SCREEN_WIDTH / RAND_MAX, rand() * SCREEN_HEIGHT / RAND_MAX);
            for(int i = 0; i < SEGMENTS_PER_APPLE; i++)
            {
                snake.push_back(snake[currentLength-1]);
                currentLength++;
            }
            ++score;
        }
        for(int i = SEGMENT_RADIUS * 2 / SEGMENT_CLIPPING_RADIUS; i < currentLength; i++)
        {
            looping &= !snake[0].isTouching(snake[i]);
        }
        angle = angle + (rightPressed - leftPressed) * TURN_SPEED;

        //Rendering
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        for(int i = 0; i<currentLength; i++)
        {
            int ic = i % 16;
            snake[i].render(renderer, circleSprite, 0, 255 - (16 * (ic - 2 * (ic > 8) * (ic - 8))), 0);
        }
        apple.render(renderer, circleSprite, 255, 0, 0);
        SDL_RenderPresent(renderer);

        //Wait for next frame (60 FPS cap)
        int msElapsed = SDL_GetTicks() - frameStart;
        if (msElapsed < 1000/60)
        {
            SDL_Delay(1000/60 - msElapsed);
        }
    }

    //Game Over effect
    int snakeSize = snake.size();
    for (int s = 0; s <= snakeSize + 1 + snakeSize / 240; s += 1 + snakeSize / 240)
    {
        int frameStart = SDL_GetTicks();
        for(int i = 0; i < currentLength; i++)
        {
            int ic = i % 16;
            int c = 255 - (16 * (ic - 2 * (ic > 8) * (ic - 8)));
            if (i > s)
            {
                snake[i].render(renderer, circleSprite, 0, c, 0);
            } else
            {
                snake[i].render(renderer, circleSprite, c, c, c);
            }
        }
        apple.render(renderer, circleSprite, 255, 0, 0);
        SDL_RenderPresent(renderer);
        int msElapsed = SDL_GetTicks() - frameStart;
        if (msElapsed < 1000/60)
        {
            SDL_Delay(1000/60 - msElapsed);
        }
    }
    std::cout << "Game Over\nScore: " << score << std::endl;
    SDL_DestroyTexture(circleSprite);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return 0;
}
