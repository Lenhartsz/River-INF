#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "raylib.h"

#define LINHAS 20
#define COLUNAS 24
#define TILE 32
#define MAX_RANK 5
#define TOTAL_FASES 5

typedef struct
{
    char nome[32];
    int pontos;
} Jogador;

typedef enum {
    TELA_MENU, TELA_NOME, TELA_JOGO, TELA_RANKING,
    TELA_FIM, TELA_VITORIA, TELA_SEM_GASOLINA,
    TELA_PAUSE, TELA_SAIR_APP
} Tela;


//-----------------------------------------------------
// MAPA / RANKING
//-----------------------------------------------------
int carregarMapa(char mapa[LINHAS][COLUNAS+1], const char *arquivo)
{
    FILE *f = fopen(arquivo, "r");
    if (!f) return 0;
    for (int i = 0; i < LINHAS; i++)
    {
        if (!fgets(mapa[i], COLUNAS+2, f))
        {
            fclose(f);
            return 0;
        }
        for (int j=0; j<COLUNAS; j++) if(mapa[i][j]=='\n') mapa[i][j]=' ';
        mapa[i][COLUNAS]='\0';
    }
    fclose(f);
    return 1;
}

void acharAviaoInicial(char mapa[LINHAS][COLUNAS+1], int *ax, int *ay)
{
    for (int y=0; y<LINHAS; y++)
        for(int x=0; x<COLUNAS; x++)
            if(mapa[y][x]=='A')
            {
                *ax=x;
                *ay=y;
                return;
            }
}

void desenharMapa(char mapa[LINHAS][COLUNAS+1],
                  Texture2D terra, Texture2D agua, Texture2D navio,
                  Texture2D fuelF, Texture2D fuelU, Texture2D fuelE, Texture2D fuelL,
                  Texture2D helicoptero)
{
    for (int y=0; y<LINHAS; y++)
        for (int x=0; x<COLUNAS; x++)
        {
            int px = x*TILE;
            int py = y*TILE;
            switch(mapa[y][x])
            {
                case 'T': DrawTexture(terra, px, py, WHITE); break;
                case 'N': DrawTexture(navio, px, py, WHITE); break;
                case 'F': DrawTexture(fuelF, px, py, WHITE); break;
                case 'U': DrawTexture(fuelU, px, py, WHITE); break;
                case 'E': DrawTexture(fuelE, px, py, WHITE); break;
                case 'L': DrawTexture(fuelL, px, py, WHITE); break;
                case 'X': DrawTexture(helicoptero, px, py, WHITE); break;
                default:  DrawTexture(agua, px, py, WHITE); break;
            }
        }
}

void salvarRanking(Jogador ranking[MAX_RANK], const char *arquivo)
{
    FILE *f = fopen(arquivo, "w");
    if(!f) return;
    for(int i=0; i<MAX_RANK; i++)
        fprintf(f,"%s %d\n", ranking[i].nome, ranking[i].pontos);
    fclose(f);
}

void carregarRanking(Jogador ranking[MAX_RANK], const char *arquivo)
{
    FILE *f = fopen(arquivo,"r");
    if(!f)
    {
        for(int i=0; i<MAX_RANK; i++)
        {
            ranking[i].nome[0]='\0';
            ranking[i].pontos=0;
        }
        return;
    }
    for(int i=0; i<MAX_RANK; i++)
        fscanf(f,"%s %d", ranking[i].nome, &ranking[i].pontos);
    fclose(f);
}

void atualizarRanking(Jogador ranking[MAX_RANK], const char *nome, int pontos)
{
    for(int i=0; i<MAX_RANK; i++)
    {
        if(pontos>ranking[i].pontos)
        {
            for(int j=MAX_RANK-1; j>i; j--) ranking[j]=ranking[j-1];
            strncpy(ranking[i].nome, nome,31);
            ranking[i].nome[31]='\0';
            ranking[i].pontos=pontos;
            break;
        }
    }
}


//-----------------------------------------------------
// COLISÕES
//-----------------------------------------------------
bool vaiColidir(char mapa[LINHAS][COLUNAS+1], float px, float py)
{
    Rectangle aviaoRec = {px, py, TILE, TILE};
    for (int y = 0; y < LINHAS; y++)
        for (int x = 0; x < COLUNAS; x++)
            if (mapa[y][x] == 'T' || mapa[y][x] == 'N' || mapa[y][x] == 'X')
            {
                Rectangle tileRec = {x*TILE, y*TILE, TILE, TILE};
                if (CheckCollisionRecs(aviaoRec, tileRec))
                    return true;
            }
    return false;
}


//-----------------------------------------------------
// MAIN
//-----------------------------------------------------
int main()
{
    InitWindow(COLUNAS*TILE, LINHAS*TILE, "River-Inf");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL); // <--- ESC não fecha mais sozinho

    Jogador ranking[MAX_RANK];
    carregarRanking(ranking, "ranking.txt");

    Tela telaAtual = TELA_NOME;
    Tela telaAnterior = TELA_MENU;   // <- usado pelo ESC global

    char nome[32]={0};
    int letraIndex=0;

    char mapa[LINHAS][COLUNAS+1];
    int ax=0,ay=0;
    float px=0,py=0;
    float velocidadeFrente=1.0f, velocidadeLateral=1.0f;
    float gasolina=100;
    int direcao=0;
    int score=0;
    float tempoScore=0;
    int faseAtual=1;
    bool fuelColetado = false;

    // Texturas
    Texture2D terra = LoadTexture("sprites/terra.png");
    Texture2D agua  = LoadTexture("sprites/agua.png");
    Texture2D navio = LoadTexture("sprites/navio.png");
    Texture2D fuelF = LoadTexture("sprites/gasolina_F.png");
    Texture2D fuelU = LoadTexture("sprites/gasolina_U.png");
    Texture2D fuelE = LoadTexture("sprites/gasolina_E.png");
    Texture2D fuelL = LoadTexture("sprites/gasolina_L.png");
    Texture2D helicoptero = LoadTexture("sprites/helicoptero.png");
    Texture2D aviao_parado  = LoadTexture("sprites/aviao_parado.png");
    Texture2D aviao_direita = LoadTexture("sprites/aviao_direita.png");
    Texture2D aviao_esquerda= LoadTexture("sprites/aviao_esquerda.png");

    //-----------------------------------------------------
    // LOOP PRINCIPAL
    //-----------------------------------------------------
    while(!WindowShouldClose())
    {
        //-------------------------------------------------
        // ESC GLOBAL → vai para TELA_SAIR_APP
        //-------------------------------------------------
        if (IsKeyPressed(KEY_ESCAPE) && telaAtual != TELA_PAUSE)
        {
            telaAnterior = telaAtual;
            telaAtual = TELA_SAIR_APP;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        switch(telaAtual)
        {

        //-------------------------------------------------
        // TELA NOME
        //-------------------------------------------------
        case TELA_NOME:
        {
            int key = GetCharPressed();
            while(key>0 && letraIndex<31)
            {
                if(key>=32 && key<=126) nome[letraIndex++] = (char)key;
                key = GetCharPressed();
            }
            if(IsKeyPressed(KEY_BACKSPACE) && letraIndex>0)
            {
                letraIndex--;
                nome[letraIndex]='\0';
            }
            if(IsKeyPressed(KEY_ENTER) && letraIndex>0)
                telaAtual = TELA_MENU;

            DrawText("Digite seu nome:", 50, 50, 20, BLACK);
            DrawText(nome, 50, 100, 30, BLUE);
            DrawText("Pressione ENTER para continuar", 50, 150, 15, DARKGRAY);
            break;
        }

        //-------------------------------------------------
        // TELA MENU
        //-------------------------------------------------
        case TELA_MENU:
        {
            static int opcao=0;
            if(IsKeyPressed(KEY_S)) opcao = (opcao+1)%3;
            if(IsKeyPressed(KEY_W)) opcao = (opcao+2)%3;

            if(IsKeyPressed(KEY_ENTER))
            {
                if(opcao==0)
                {
                    // reset jogo
                    faseAtual=1;
                    score=0;
                    gasolina=100;
                    tempoScore=0;
                    fuelColetado=false;

                    char nomeFase[64];
                    sprintf(nomeFase, "mapas/fase%d.txt", faseAtual);

                    carregarMapa(mapa, nomeFase);
                    acharAviaoInicial(mapa,&ax,&ay);
                    px=ax*TILE;
                    py=ay*TILE;

                    telaAtual = TELA_JOGO;
                }
                else if(opcao==1)
                    telaAtual=TELA_RANKING;
                else
                    CloseWindow();
            }

            DrawText("RIVER-INF\n\n\n\n\n\n\n\n\n           By @lenhartt1", 120,50,30,BLUE);
            const char* ops[]={"Comecar Jogo","Ranking","Sair"};

            for(int i=0; i<3; i++)
            {
                Color c=(i==opcao)?RED:BLACK;
                DrawText(ops[i],150,150+i*40,20,c);
            }
            break;
        }


        //-------------------------------------------------
        // TELA RANKING
        //-------------------------------------------------
        case TELA_RANKING:
        {
            DrawText("RANKING TOP 5",100,10,20,BLACK);

            for(int i=0;i<MAX_RANK;i++)
                DrawText(TextFormat("%d. %s - %d",i+1,ranking[i].nome,ranking[i].pontos),
                         50,50+i*40,20,DARKGRAY);

            DrawText("Pressione ENTER para voltar",50,250,15,RED);
            if(IsKeyPressed(KEY_ENTER)) telaAtual=TELA_MENU;
            break;
        }


        //-------------------------------------------------
        // TELA JOGO
        //-------------------------------------------------
        case TELA_JOGO:
        {
            float dt=GetFrameTime();
            tempoScore+=dt;

            if(tempoScore>=1)
            {
                score+=10;
                tempoScore=0;
            }

            bool tAlt=false;
            direcao=0;

            // ENTER = pause
            if(IsKeyPressed(KEY_ENTER))
            {
                telaAtual=TELA_PAUSE;
                break;
            }

            // A/D
            if(IsKeyDown(KEY_A))
            {
                float nx=px-velocidadeLateral;
                if(vaiColidir(mapa,nx,py)) telaAtual=TELA_FIM;
                else {px=nx; direcao=-1;}
            }
            else if(IsKeyDown(KEY_D))
            {
                float nx=px+velocidadeLateral;
                if(vaiColidir(mapa,nx,py)) telaAtual=TELA_FIM;
                else {px=nx; direcao=1;}
            }

            // W/S
            float velF=velocidadeFrente;
            if(IsKeyDown(KEY_S)) velF*=0.5f;
            if(IsKeyDown(KEY_W)) velF*=2;

            float ny=py-velF;
            if(vaiColidir(mapa,px,ny)) telaAtual=TELA_FIM;
            else py=ny;

            // Combustível
            gasolina -= 5*dt;
            if(gasolina<=0)
            {
                gasolina=0;
                telaAtual=TELA_SEM_GASOLINA;
            }

            // Coleta fuel
            if(!fuelColetado)
            {
                int tx=px/TILE;
                int ty=py/TILE;
                char tile = mapa[ty][tx];

                if(tile=='F'||tile=='U'||tile=='E'||tile=='L')
                {
                    if(IsKeyPressed(KEY_W))
                    {
                        score+=500;
                        fuelColetado=true;
                    }
                    else if(IsKeyPressed(KEY_S))
                    {
                        gasolina+=20;
                        if(gasolina>100) gasolina=100;
                        fuelColetado=true;
                    }
                }
            }

            // Próxima fase
            if(py<=0)
            {
                score+=1000;
                faseAtual++;

                if(faseAtual > TOTAL_FASES)
                {
                    telaAtual=TELA_VITORIA;
                }
                else
                {
                    char faseN[64];
                    sprintf(faseN, "mapas/fase%d.txt", faseAtual);

                    carregarMapa(mapa,faseN);
                    acharAviaoInicial(mapa,&ax,&ay);
                    px=ax*TILE;
                    py=ay*TILE;
                    fuelColetado=false;
                }
            }

            // Desenho
            desenharMapa(mapa, terra, agua, navio, fuelF, fuelU, fuelE, fuelL, helicoptero);

            Texture2D avi =
                (direcao==1)?aviao_direita :
                (direcao==-1)?aviao_esquerda :
                aviao_parado;

            DrawTexture(avi,px,py,WHITE);

            // Barra de combustível
            float maxB=200;
            float h=20;
            float prop=gasolina/100;
            float filled=maxB*prop;

            Color c=GREEN;
            if(gasolina<60) c=YELLOW;
            if(gasolina<30) c=RED;

            DrawRectangle(10,10,maxB,h,GRAY);
            DrawRectangle(10,10,filled,h,c);
            DrawRectangleLines(10,10,maxB,h,BLACK);
            DrawText(TextFormat("%.0f%%",gasolina), 220,10,20,BLACK);

            // Score
            DrawText(TextFormat("Score: %d",score), 10,40,25,RED);

            break;
        }


        //-------------------------------------------------
        // TELA PAUSE
        //-------------------------------------------------
        case TELA_PAUSE:
        {
            DrawRectangle(0,0,COLUNAS*TILE,LINHAS*TILE, Fade(BLACK,0.5f));
            DrawText("                    PAUSE", 100,50,40,YELLOW);

            static int opc=0;
            const char* ops[]={"Continuar","Voltar ao menu"};

            if(IsKeyPressed(KEY_S)) opc=(opc+1)%2;
            if(IsKeyPressed(KEY_W)) opc=(opc+1)%2;

            for(int i=0;i<2;i++)
            {
                Color c=(i==opc)?RED:WHITE;
                DrawText(ops[i],150,150+i*40,30,c);
            }

            if(IsKeyPressed(KEY_ENTER))
            {
                if(opc==0) telaAtual=TELA_JOGO;
                else
                {
                    atualizarRanking(ranking,nome,score);
                    salvarRanking(ranking,"ranking.txt");
                    telaAtual=TELA_MENU;
                }
            }
            break;
        }


        //-------------------------------------------------
        // TELA GAME OVER
        //-------------------------------------------------
        case TELA_FIM:
        {
            DrawText("BOOM! Fim de fase!",50,50,30,RED);
            DrawText(TextFormat("Score: %d",score),50,100,25,BLACK);
            DrawText("Press ENTER para menu",50,150,15,DARKGRAY);

            if(IsKeyPressed(KEY_ENTER))
            {
                atualizarRanking(ranking,nome,score);
                salvarRanking(ranking,"ranking.txt");
                telaAtual=TELA_MENU;
            }
            break;
        }


        //-------------------------------------------------
        // TELA VITORIA
        //-------------------------------------------------
        case TELA_VITORIA:
        {
            DrawText("PARABENS! TODAS AS FASES!",50,50,30,GREEN);
            DrawText(TextFormat("Score: %d",score),50,100,25,BLACK);

            DrawText("Press ENTER para voltar",50,150,15,DARKGRAY);

            if(IsKeyPressed(KEY_ENTER))
            {
                atualizarRanking(ranking,nome,score);
                salvarRanking(ranking,"ranking.txt");
                telaAtual=TELA_MENU;
            }
            break;
        }


        //-------------------------------------------------
        // TELA FALTA GASOLINA
        //-------------------------------------------------
        case TELA_SEM_GASOLINA:
        {
            DrawText("ACABOU GASOLINA!",50,50,30,ORANGE);
            DrawText(TextFormat("Score: %d",score),50,100,25,BLACK);

            DrawText("Press ENTER para voltar",50,150,15,DARKGRAY);

            if(IsKeyPressed(KEY_ENTER))
            {
                atualizarRanking(ranking,nome,score);
                salvarRanking(ranking,"ranking.txt");
                telaAtual=TELA_MENU;
            }
            break;
        }


        //-------------------------------------------------
        // *** TELA SAIR DO APLICATIVO (ESC GLOBAL) ***
        //-------------------------------------------------
        case TELA_SAIR_APP:
        {
            DrawRectangle(0,0,COLUNAS*TILE,LINHAS*TILE, Fade(BLACK,0.6f));

            DrawText("Deseja realmente sair?", 50, 50, 30, YELLOW);

            static int opc=0;
            const char* ops[]={"NAO","SIM"};

            if(IsKeyPressed(KEY_S)) opc=(opc+1)%2;
            if(IsKeyPressed(KEY_W)) opc=(opc+1)%2;

            for(int i=0;i<2;i++)
            {
                Color c=(i==opc)?RED:WHITE;
                DrawText(ops[i],150,150+i*40,30,c);
            }

            if(IsKeyPressed(KEY_ENTER))
            {
                if(opc==0)
                    telaAtual=telaAnterior; // volta para antes
                else
                    CloseWindow();          // fecha app
            }
            break;
        }

        }

        EndDrawing();
    }

    UnloadTexture(terra);
    UnloadTexture(agua);
    UnloadTexture(navio);
    UnloadTexture(fuelF);
    UnloadTexture(fuelU);
    UnloadTexture(fuelE);
    UnloadTexture(fuelL);
    UnloadTexture(helicoptero);
    UnloadTexture(aviao_parado);
    UnloadTexture(aviao_direita);
    UnloadTexture(aviao_esquerda);

    CloseWindow();
    return 0;
}
