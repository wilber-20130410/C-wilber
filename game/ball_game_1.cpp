/*
名称:ball_game_1.cpp(弹球游戏)
作者:wilber-20130410
版权: © 2025~2026 wilber-20130410
版本:1.0.0[133000916195301](正式版)
日期:2025.9.16
留言:
1.本代码仅供学习交流使用,请勿用于商业用途。
2.本代码参考了网络上部分代码,在此表示感谢。
3.使用前请确保已经安装以下所使用的库。
4.本人推荐使用Visual Studio Code作为IDE(集成开发环境)。
5.如果您有建议、发现了Bug、问题或者您进行优化后的代码,欢迎向本人邮箱xuwb0410@163.com发送邮件,本人将在21天内进行回复。
6.本代码适用于Linux(ubuntu)系统。
7.以上留言不分先后。
*/

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <string>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int BALL_SIZE = 15;
const int PADDLE_WIDTH = 100;
const int PADDLE_HEIGHT = 20;

struct GameState {
    SDL_Rect ball = {SCREEN_WIDTH/2, SCREEN_HEIGHT/2, BALL_SIZE, BALL_SIZE};
    SDL_Rect paddle = {SCREEN_WIDTH/2-PADDLE_WIDTH/2, SCREEN_HEIGHT-50, PADDLE_WIDTH, PADDLE_HEIGHT};
    int ballSpeedX = 5;
    int ballSpeedY = -5;
    int score = 0;
    bool running = true;
};

int main(int argc, char* argv[]) {
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL初始化失败: " << SDL_GetError() << std::endl;
        return 1;
    }
    if(TTF_Init() == -1) {
        std::cerr << "TTF初始化失败: " << TTF_GetError() << std::endl;
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("SDL弹球游戏", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font* font = TTF_OpenFont("arial.ttf", 24);
    if(!font) {
        std::cerr << "字体加载失败: " << TTF_GetError() << std::endl;
    }

    GameState game;
    SDL_Event e;

    while(game.running) {
        while(SDL_PollEvent(&e) != 0) {
            if(e.type == SDL_QUIT) {
                game.running = false;
            }
        }
        const Uint8* keystates = SDL_GetKeyboardState(NULL);
        if(keystates[SDL_SCANCODE_LEFT] && game.paddle.x > 0) {
            game.paddle.x -= 10;
        }
        if(keystates[SDL_SCANCODE_RIGHT] && game.paddle.x < SCREEN_WIDTH - game.paddle.w) {
            game.paddle.x += 10;
        }
        // 更新球位置
        game.ball.x += game.ballSpeedX;
        game.ball.y += game.ballSpeedY;
        // 碰撞检测
        if(game.ball.x <= 0 || game.ball.x >= SCREEN_WIDTH - game.ball.w) {
            game.ballSpeedX = -game.ballSpeedX;
        }
        if(game.ball.y <= 0) {
            game.ballSpeedY = -game.ballSpeedY;
        }
        // 挡板碰撞
        if(SDL_HasIntersection(&game.ball, &game.paddle)) {
            game.ballSpeedY = -game.ballSpeedY;
            game.score += 10;
        }
        // 游戏结束
        if(game.ball.y >= SCREEN_HEIGHT) {
            game.running = false;
        }
        // 渲染
        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &game.ball);
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderFillRect(renderer, &game.paddle);
        // 显示分数
        if(font) {
            std::string scoreText = "得分: " + std::to_string(game.score);
            SDL_Color textColor = {0, 0, 0, 255};
            SDL_Surface* surface = TTF_RenderText_Solid(font, scoreText.c_str(), textColor);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect textRect = {10, 10, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &textRect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60FPS
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
