#include "raylib.h"
#include <stdlib.h>
#define LARGURA 1368
#define ALTURA 768
#define MAX_SOUNDS 10

Sound soundArray[MAX_SOUNDS] = { 0 };
int currentSound;


int main(void)
{

    InitAudioDevice();
    InitWindow(LARGURA, ALTURA, "Aviao com Sprite");
    SetTargetFPS(60);

    // Carreg a sprite do fundo do game
    Texture2D fundo = LoadTexture("sprites/fundo.png");
    // Carrega o sprite (ex: "sprites/aviaoreto.png")
    Texture2D aviao = LoadTexture("sprites/aviaoreto.png");
    // Carrega o som base do aviao
    Sound aviaoSom = LoadSound("aviaosom.wav");
    // Posição inicial do avião
    float eixo_x = 700;
    float eixo_y = 728;


    float velocidadeSubida = -90.0f;

    // Como a imagem é o avião inteiro, usamos ela toda:
    Rectangle source = { 0, 0, (float)aviao.width, (float)aviao.height };

    // Onde desenhar na tela, e o tamanho
    Rectangle dest = { eixo_x, eixo_y, 64, 64 };

    //PlaySound(aviaoSom);

    while (!WindowShouldClose())
    {

        // Loop do som
        // if (!IsSoundPlaying(aviaoSom))
          //  PlaySound(aviaoSom);

         float dt = GetFrameTime();
        // --- velocidade constante ---

        eixo_y += velocidadeSubida * dt;

        // --- Movimento ---
        if (IsKeyDown(KEY_D)){ eixo_x += 4;
        }
        if (IsKeyDown(KEY_A)){ eixo_x -= 4;
        }
        if (IsKeyDown(KEY_W)) {eixo_y -= 2;
        }
        if (IsKeyDown(KEY_S)) eixo_y {
                += 1; // ir para tras só diminui a velocidade
        }
        //  manter o quadrado e o png do aviao no mesmo lugar
        dest.x = eixo_x;
        dest.y = eixo_y;

        // --- Desenho ---
        BeginDrawing();
        DrawTexture(fundo, 0, 0, WHITE);

        DrawTexturePro(aviao, source, dest, (Vector2){0, 0}, 0.0f, WHITE);
        if(eixo_y<-75) CloseWindow(); // fechar a janela assim que o aviao sai da tela

        //DrawRectangleLines(dest.x, dest.y, dest.width, dest.height, GREEN); Usar para pegar uma referencia de HitBox

        EndDrawing();
    }
    UnloadTexture(fundo);
    UnloadTexture(aviao);
    CloseWindow();
    return 0;
}
