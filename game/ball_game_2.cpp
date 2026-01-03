/*
名称:ball_game_2.cpp(弹球游戏)
作者:wilber-20130410
版权: © 2025~2026 wilber-20130410
版本:1.0.0[133000916195601](正式版)
日期:2025.9.16
留言:
1.本代码仅供学习交流使用,请勿用于商业用途。
2.本代码参考了网络上部分代码,在此表示感谢。
3.使用前请确保已经安装以下所使用的库。
4.本人推荐使用Visual Studio Code作为IDE(集成开发环境)。
5.如果您有建议、发现了Bug、问题或者您进行优化后的代码,欢迎向本人邮箱xuwb0410@163.com发送邮件,本人将在21天内进行回复。
6.本代码适用于Windows系统。
7.以上留言不分先后。
*/

#include <graphics.h>
#include <conio.h>
#include <time.h>

#define WIDTH 800
#define HEIGHT 600
#define BALL_RADIUS 15
#define PADDLE_WIDTH 100
#define PADDLE_HEIGHT 20

struct Ball {
    int x, y;
    int dx, dy;
    COLORREF color;
};

struct Paddle {
    int x, y;
    int width;
};

int main() {
    initgraph(WIDTH, HEIGHT);
    setbkcolor(RGB(240, 240, 240));
    cleardevice();
    // 初始化小球
    Ball ball = {
        WIDTH / 2,
        HEIGHT / 2,
        5,
        -5,
        RGB(255, 0, 0)
    };
    // 初始化挡板
    Paddle paddle = {
        WIDTH / 2 - PADDLE_WIDTH / 2,
        HEIGHT - 50,
        PADDLE_WIDTH
    };
    int score = 0;
    bool gameOver = false;
    srand(time(NULL));

    while (!gameOver) {
        // 清屏
        cleardevice();
        // 处理键盘输入
        if (_kbhit()) {
            char key = _getch();
            if (key == 'a' && paddle.x > 0) {
                paddle.x -= 15;
            }
            if (key == 'd' && paddle.x < WIDTH - paddle.width) {
                paddle.x += 15;
            }
        }
        // 更新小球位置
        ball.x += ball.dx;
        ball.y += ball.dy;
        // 碰撞检测 - 墙壁
        if (ball.x <= BALL_RADIUS || ball.x >= WIDTH - BALL_RADIUS) {
            ball.dx = -ball.dx;
        }
        if (ball.y <= BALL_RADIUS) {
            ball.dy = -ball.dy;
        }
        // 碰撞检测 - 挡板
        if (ball.y >= HEIGHT - BALL_RADIUS - PADDLE_HEIGHT &&
            ball.x >= paddle.x &&
            ball.x <= paddle.x + paddle.width) {
            ball.dy = -ball.dy;
            score += 10;
            // 随机改变小球颜色
            ball.color = RGB(rand() % 256, rand() % 256, rand() % 256);
        }
        // 游戏结束判断
        if (ball.y >= HEIGHT) {
            gameOver = true;
        }
        // 绘制元素
        setfillcolor(ball.color);
        fillcircle(ball.x, ball.y, BALL_RADIUS);
        setfillcolor(RGB(0, 0, 255));
        fillrectangle(paddle.x, paddle.y, paddle.x + paddle.width, paddle.y + PADDLE_HEIGHT);
        // 显示分数
        settextstyle(20, 0, _T("宋体"));
        settextcolor(RGB(0, 0, 0));
        char scoreText[20];
        sprintf(scoreText, "得分: %d", score);
        outtextxy(10, 10, scoreText);
        // 控制帧率
        Sleep(30);
    }
    // 游戏结束显示
    settextstyle(40, 0, _T("宋体"));
    settextcolor(RGB(255, 0, 0));
    outtextxy(WIDTH / 2 - 100, HEIGHT / 2 - 20, _T("游戏结束!"));
    outtextxy(WIDTH / 2 - 80, HEIGHT / 2 + 30, scoreText);
    _getch();
    closegraph();
    return 0;
}
