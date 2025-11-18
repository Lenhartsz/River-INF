#include "raylib.h"

int main(void)
{
    InitWindow(800, 600, "River Raid Sprite Test");
    SetTargetFPS(60);

    Texture2D spritesheet = LoadTexture("sprites.png");

    // Sprite do jogador (avião amarelo à esquerda)
    // Cada sprite parece ter cerca de 16x16 pixels (ajusta se necessário)
    Rectangle source = { 10, 12, 16, 16 }; // região dentro da imagem
    Vector2 position = { 400, 300 };       // onde desenhar na tela

    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(BLUE);

            // desenha apenas o retângulo definido em 'source'
            DrawTextureRec(spritesheet, source, position, WHITE);

            DrawText("Sprite do jogador do River Raid", 10, 10, 20, RAYWHITE);
        EndDrawing();
    }

    UnloadTexture(spritesheet);
    CloseWindow();

    return 0;
}
