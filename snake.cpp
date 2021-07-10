
#include <stdint.h>

typedef uint32_t u32;
typedef  uint8_t  u8;
typedef  int32_t s32;

struct v2s   { s32 X, Y; };
struct color { u8 R, G, B; };

#define ArrayCount(x) (sizeof(x)/sizeof(*(x)))

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define GRID_COUNT_X        20
#define GRID_COUNT_Y        20
#define GRID_ELEMENT_SIZE_X 40
#define GRID_ELEMENT_SIZE_Y 40
#define BORDER_SIZE         20
#define TICK_DIVIDER        10
#define FOOD_COUNT          10

enum struct grid_element
{
    empty,
    snake,
    food,
};

enum struct direction
{
    X, X_,
    Y, Y_,
    invalid,
};

// NOTE: snake should maybe be a doubly-linked list so that we don't have to go through every elemnt each time we search for tail
struct snake
{
    v2s P;
    snake *Next;
};

static grid_element Grid[GRID_ELEMENT_SIZE_X][GRID_ELEMENT_SIZE_Y];

static snake    *Snake;
static direction SnakeDirection;
static direction TailDirection;
static bool      Won;
static bool      QuitRequested;

static inline grid_element &GridAt(v2s P) { return Grid[P.X][P.Y]; };

#include <SDL2/SDL.h>

static SDL_Renderer *Renderer;
static SDL_Window   *Window;

static void
PlaceFood()
{
    // It would be better if we counted how many spots are free (F),
    // then chosen a random number N up to F,
    // and pick the N'th free spot...
    for(;;)
    {
        v2s P =
        {
            .X = rand() % GRID_COUNT_X,
            .Y = rand() % GRID_COUNT_X,
        };
        if(GridAt(P) == grid_element::empty)
        {
            GridAt(P) = grid_element::food;
            break;
        }
    }
}

static direction
FindTailDirection()
{
    if(Snake->Next == NULL) return direction::invalid;
    s32 X = Snake->P.X - Snake->Next->P.X;
    s32 Y = Snake->P.Y - Snake->Next->P.Y;
    if(X ==  1) return direction::X_;
    if(X == -1) return direction::X;
    if(Y ==  1) return direction::Y_;
    if(Y == -1) return direction::Y;
    return direction::invalid;
}

static bool
GridSpotAvailable()
{
    for(s32 X = 0; X < GRID_COUNT_X; ++X)
    {
        for(s32 Y = 0; Y < GRID_COUNT_Y; ++Y)
        {
            if(Grid[X][Y] == grid_element::empty) return true;
        }
    }
    return false;
}

static bool
Tick()
{
    v2s NextP = Snake->P;
    switch(SnakeDirection)
    {
        case direction::X:  NextP.X += 1; break;
        case direction::X_: NextP.X -= 1; break;
        case direction::Y:  NextP.Y += 1; break;
        case direction::Y_: NextP.Y -= 1; break;
        default: assert(0);
    }
    if(NextP.X >= GRID_COUNT_X || NextP.X < 0 ||
       NextP.Y >= GRID_COUNT_Y || NextP.Y < 0)
    {
        return true;
    }
    auto G = GridAt(NextP);
    switch(G)
    {
        
        case grid_element::snake:
        {
            return true;
        } break;
        
        case grid_element::empty:
        {
            snake **Last = NULL;
            for(auto At = &Snake;
                *At;
                At = &(*At)->Next)
            {
                Last = At;
            }
            snake *Elem = *Last;
            assert(Elem->Next == NULL);
            GridAt(Elem->P) = grid_element::empty;
            *Last = NULL;
            Elem->Next = Snake;
            Snake = Elem;
            Elem->P = NextP;
            GridAt(NextP) = grid_element::snake;
        } break;
        
        case grid_element::food:
        {
            auto NewHead = (snake *)malloc(sizeof(snake));
            NewHead->P = NextP;
            NewHead->Next = Snake;
            Snake = NewHead;
            GridAt(NextP) = grid_element::snake;
            if(GridSpotAvailable())
            {
                PlaceFood();
            } else
            {
                Won = true;
                return true;
            }
        } break;
        
        default: assert(0); break;
    }
    return false;
}

static void
DrawPiece(v2s Position, color Color)
{
    SDL_Rect Rect;
    Rect.x = BORDER_SIZE+Position.X*(GRID_ELEMENT_SIZE_X);
    Rect.y = BORDER_SIZE+Position.Y*(GRID_ELEMENT_SIZE_Y);
    Rect.w = GRID_ELEMENT_SIZE_X;
    Rect.h = GRID_ELEMENT_SIZE_Y;
    SDL_SetRenderDrawColor(Renderer, Color.R,Color.G,Color.B, 255);
    SDL_RenderFillRect(Renderer, &Rect);
}

static void
Draw()
{
    { // DrawBorder
        SDL_Rect Rect;
        Rect.x = 0;
        Rect.y = 0;
        Rect.w = GRID_ELEMENT_SIZE_X*GRID_COUNT_X + 2*BORDER_SIZE;
        Rect.h = GRID_ELEMENT_SIZE_Y*GRID_COUNT_Y + 2*BORDER_SIZE;
        SDL_SetRenderDrawColor(Renderer, 50,50,50, 255);
        SDL_RenderFillRect(Renderer, &Rect);
        Rect.x = BORDER_SIZE;
        Rect.y = BORDER_SIZE;
        Rect.w = GRID_ELEMENT_SIZE_X*GRID_COUNT_X;
        Rect.h = GRID_ELEMENT_SIZE_Y*GRID_COUNT_Y;
        SDL_SetRenderDrawColor(Renderer, 200,200,200, 255);
        SDL_RenderFillRect(Renderer, &Rect);
    }
    for(s32 X = 0; X < GRID_COUNT_X; ++X)
    {
        for(s32 Y = 0; Y < GRID_COUNT_Y; ++Y)
        {
            static color Colors[]
            {
                [(u32)grid_element::empty] = {255, 255, 255},
                [(u32)grid_element::snake] = {  0,   0,   0},
                [(u32)grid_element::food]  = {255,   0,   0},
            };
            auto Color = Colors[(u32)Grid[X][Y]];
            DrawPiece({X,Y}, Color);
        }
    }
    DrawPiece(Snake->P, {0,150,0});
}

static void
ProcessEvents()
{
    SDL_Event Event;
    while(SDL_PollEvent(&Event))
    {
        switch(Event.type)
        {
            case SDL_QUIT: QuitRequested = true; break;
            
            case SDL_KEYDOWN:
            {
                u32 Key = Event.key.keysym.scancode;
                direction NewDirection  = direction::invalid;
                
                ;    if(Key == SDL_SCANCODE_ESCAPE) QuitRequested = true;
                else if(Key == SDL_SCANCODE_RIGHT) NewDirection = direction::X;
                else if(Key == SDL_SCANCODE_LEFT)  NewDirection = direction::X_;
                else if(Key == SDL_SCANCODE_DOWN)  NewDirection = direction::Y;
                else if(Key == SDL_SCANCODE_UP)    NewDirection = direction::Y_;
                if((NewDirection != direction::invalid) &&
                   TailDirection != NewDirection)
                {
                    SnakeDirection = NewDirection;
                }
            } break;
            
            case SDL_WINDOWEVENT:
            {
                if(Event.window.windowID != SDL_GetWindowID(Window)) break;
                switch(Event.window.event)
                {
                    case SDL_WINDOWEVENT_CLOSE: QuitRequested = true;  break;
                }
            } break;
            
        }
    }
}

s32
main()
{
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL error: %s\n", SDL_GetError());
        return 1;
    }
    
#define WINDOW_SIZE_X GRID_COUNT_X*GRID_ELEMENT_SIZE_X + 2*BORDER_SIZE
#define WINDOW_SIZE_Y GRID_COUNT_Y*GRID_ELEMENT_SIZE_Y + 2*BORDER_SIZE
    Window   = SDL_CreateWindow("Snake", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_SIZE_X, WINDOW_SIZE_Y, SDL_WINDOW_BORDERLESS);
    Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_PRESENTVSYNC);
    
    Snake = (snake *)malloc(sizeof(snake));
    *Snake =
    {
        .P    = {GRID_COUNT_X / 2, GRID_COUNT_Y / 2},
        .Next = NULL,
    };
    Grid[Snake->P.X][Snake->P.Y] = grid_element::snake;
    
    for(u32 i = 0; i < FOOD_COUNT; ++i) PlaceFood();
    
    u32 FrameAt = 0;
    bool Running = true;
    while(Running)
    {
        ProcessEvents();
        if(QuitRequested) break;
        
        if((++FrameAt % TICK_DIVIDER) == 0)
        {
            if(Tick() == true)
            {
                if(Won == true)
                {
                    printf("You won\n");
                } else
                {
                    printf("You lost\n");
                }
                u32 Length = 0;
                for(auto At = Snake;
                    At;
                    At = At->Next)
                {
                    ++Length;
                }
                printf("Snake length: %u\n", Length);
                break;
            }
            TailDirection = FindTailDirection();
        }
        Draw();
        SDL_RenderPresent(Renderer);
    }
    
    SDL_DestroyWindow(Window);
    SDL_Quit();
    return 0;
}
