#include "raylib.h"
#include <stdlib.h>

#define LARGURA 1368
#define ALTURA 768

int main(void)
{
    InitWindow(LARGURA, ALTURA, "Aviao com Sprite");
    SetTargetFPS(60);

    // Carrega o sprite (ex: "sprites/aviaoreto.png")
    Texture2D aviao = LoadTexture("sprites/aviaoreto.png");

    // Posição inicial do avião
    float eixo_x = 700;
    float eixo_y = 700;


    float velocidadeSubida = -100.0f;

    // Como a imagem é o avião inteiro, usamos ela toda:
    Rectangle source = { 0, 0, (float)aviao.width, (float)aviao.height };

    // Onde desenhar na tela, e o tamanho
    Rectangle dest = { eixo_x, eixo_y, 64, 64 };

    while (!WindowShouldClose())
    {
         float dt = GetFrameTime();
        // --- velocidade constante ---

        eixo_y += velocidadeSubida * dt;

        // --- Movimento ---
        if (IsKeyDown(KEY_D)) eixo_x += 4;
        if (IsKeyDown(KEY_A)) eixo_x -= 4;
        if (IsKeyDown(KEY_W)) eixo_y -= 4;
        if (IsKeyDown(KEY_S)) eixo_y += 1; // ir para tras só diminui a velocidade

        //  manter o quadrado e o png do aviao no mesmo lugar
        dest.x = eixo_x;
        dest.y = eixo_y;

        // --- Desenho ---
        BeginDrawing();
       ClearBackground(WHITE);

        DrawTexturePro(aviao, source, dest, (Vector2){0, 0}, 0.0f, WHITE);


        //DrawRectangleLines(dest.x, dest.y, dest.width, dest.height, GREEN); Usar para pegar uma referencia de HitBox

        EndDrawing();
    }

    UnloadTexture(aviao);
    CloseWindow();
    return 0;
}
