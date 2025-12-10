#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "raylib.h"

#define LINHAS 20
#define COLUNAS 24
#define TILE 32
#define MAX_RANK 5
#define TOTAL_FASES 5

#define MAX_BALAS 32
#define MAX_EXPLOSOES 32

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

typedef struct {
    float x, y;
    float w, h;
    bool ativa;
} Bala;

typedef struct {
    float x, y;        // posição em pixels (centro/alinhado quando criado)
    bool ativa;
    float tempo;      // tempo passado
    float duracao;    // duração total da animação
    int frameAtual;
    int totalFrames;
} Explosao;


// globals para sprite de explosão (configurados em main)
static int g_expl_totalFrames = 0;
static int g_expl_frameW = 0;
static int g_expl_spriteH = 0;


// Utility: texto com sombra
static void DrawTextShadow(const char *txt, int x, int y, int size, Color col) {
    DrawText(txt, x+2, y+2, size, BLACK);
    DrawText(txt, x, y, size, col);
}

static void DrawTextShadowFormat(int x, int y, int size, Color col, const char *format, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    DrawTextShadow(buf, x, y, size, col);
}


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
                  Texture2D helicoptero, Texture2D ponte)
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
                case 'P': DrawTexture(ponte, px, py, WHITE); break;
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
// Explosões: função C para criar (posição em pixels)
//-----------------------------------------------------
void criarExplosao(Explosao explosoes[], int maxExpl, float x, float y, float duracao, int totalFrames)
{
    for (int i=0;i<maxExpl;i++){
        if(!explosoes[i].ativa){
            explosoes[i].ativa = true;
            explosoes[i].x = x;
            explosoes[i].y = y;
            explosoes[i].tempo = 0.0f;
            explosoes[i].duracao = duracao;
            explosoes[i].frameAtual = 0;
            explosoes[i].totalFrames = totalFrames;
            return;
        }
    }
}


//-----------------------------------------------------
// MAIN
//-----------------------------------------------------
int main()
{
    InitWindow(COLUNAS*TILE, LINHAS*TILE, "River-Inf");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    Jogador ranking[MAX_RANK];
    carregarRanking(ranking, "ranking.txt");

    Tela telaAtual = TELA_NOME;
    Tela telaAnterior = TELA_MENU;

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

    // ------- TEXTURAS -------
    Texture2D ponte = LoadTexture("sprites/ponte.png");
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

    // explosão sprite (sprite sheet horizontal)
    Texture2D explosaoSprite = LoadTexture("sprites/explosion.png");
    g_expl_totalFrames = 8; // ajuste caso sua sprite tenha outro número de frames
    if (g_expl_totalFrames <= 0) g_expl_totalFrames = 1;
    g_expl_frameW = (int)(explosaoSprite.width / g_expl_totalFrames);
    g_expl_spriteH = explosaoSprite.height;

    // --- Balas ---
    Bala balas[MAX_BALAS];
    for (int i=0;i<MAX_BALAS;i++) { balas[i].ativa = false; balas[i].w = 4; balas[i].h = 10; }
    float cadenciaTiro = 1.0f;
    float timerTiro = 0.0f;
    float velocidadeBala = 300.0f;

    // --- Explosões ---
    Explosao explosoes[MAX_EXPLOSOES];
    for (int i=0;i<MAX_EXPLOSOES;i++) explosoes[i].ativa = false;

    // Paleta "militar"
    Color MIL_OLIVE = (Color){89, 99, 71, 255};
    Color MIL_DARK = (Color){40, 45, 36, 255};
    Color MIL_ACCENT = (Color){209, 167, 81, 255}; // amarelo-militar
    Color MIL_BORDER = (Color){30, 30, 25, 255};

    //-----------------------------------------------------
    // LOOP PRINCIPAL
    //-----------------------------------------------------
    timerTiro = cadenciaTiro; // permite tiro imediato se quiser

    while(!WindowShouldClose())
    {
        // ESC GLOBAL
        if (IsKeyPressed(KEY_ESCAPE) && telaAtual != TELA_PAUSE && telaAtual != TELA_SAIR_APP)
        {
            telaAnterior = telaAtual;
            telaAtual = TELA_SAIR_APP;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        int sw = GetScreenWidth();
        int sh = GetScreenHeight();

        switch(telaAtual)
        {

        // ============================================================
        // TELA_NOME
        // ============================================================
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

            DrawRectangle(0,0,sw,sh, Fade(BLACK,0.35f));
            int w = 520, h = 260;
            int x = sw/2 - w/2;
            int y = sh/2 - h/2;
            DrawRectangle(x-6,y-6,w+12,h+12,MIL_BORDER);
            DrawRectangle(x,y,w,h,MIL_DARK);

            DrawTextShadow("RIVER-INF — IDENTIFICACAO", x+20, y+18, 24, MIL_ACCENT);
            DrawText("Digite seu nome (ENTER para confirmar):", x+20, y+70, 18, LIGHTGRAY);

            int bx = x+20, by = y+110, bw = w-40, bh = 40;
            DrawRectangle(bx-3, by-3, bw+6, bh+6, MIL_OLIVE);
            DrawRectangle(bx, by, bw, bh, (Color){20,20,20,255});
            DrawText(nome, bx+8, by+6, 22, MIL_ACCENT);

            DrawTextShadow("Pressione ENTER para continuar", x+20, y+h-40, 16, LIGHTGRAY);
            break;
        }


        // ============================================================
        // MENU
        // ============================================================
        case TELA_MENU:
        {
            static int opcao = 0;
            if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) opcao = (opcao+1)%3;
            if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) opcao = (opcao+2)%3;

            if(IsKeyPressed(KEY_ENTER))
            {
                if(opcao==0)
                {
                    // Reset jogo
                    faseAtual=1;
                    score=0;
                    gasolina=100;
                    tempoScore=0;
                    fuelColetado=false;

                    char nomeFase[64];
                    sprintf(nomeFase, "mapas/fase%d.txt", faseAtual);

                    if (!carregarMapa(mapa, nomeFase)) {
                        // se falhar, inicializa mapa vazio
                        for (int r=0;r<LINHAS;r++) {
                            for (int c=0;c<COLUNAS;c++) mapa[r][c] = ' ';
                            mapa[r][COLUNAS]='\0';
                        }
                    }
                    acharAviaoInicial(mapa,&ax,&ay);
                    px=ax*TILE;
                    py=ay*TILE;

                    for (int i=0;i<MAX_BALAS;i++) balas[i].ativa=false;
                    for (int i=0;i<MAX_EXPLOSOES;i++) explosoes[i].ativa=false;

                    timerTiro = cadenciaTiro;

                    telaAtual = TELA_JOGO;
                }
                else if(opcao==1)
                    telaAtual=TELA_RANKING;
                else
                    CloseWindow();
            }

            DrawRectangle(0,0,sw,sh, Fade(BLACK,0.15f));
            int w = 520, h = 360, x = sw/2 - w/2, y = sh/2 - h/2;
            DrawRectangle(x-6,y-6,w+12,h+12, MIL_BORDER);
            DrawRectangle(x,y,w,h, MIL_DARK);

            DrawTextShadow("RIVER-INF", x+28, y+20, 48, MIL_ACCENT);
            DrawText("By @lenhartt1", x+30, y+80, 18, LIGHTGRAY);

            const char* ops[] = {"Comecar Jogo", "Ranking", "Sair"};
            for (int i=0;i<3;i++)
            {
                int tx = x + 60;
                int ty = y + 140 + i*60;
                Color col = (i==opcao) ? MIL_ACCENT : LIGHTGRAY;
                DrawRectangle(tx-10, ty-6, 400, 48, (i==opcao) ? (Color){30,40,30,255} : (Color){18,18,18,255});
                DrawRectangleLines(tx-10, ty-6, 400, 48, MIL_BORDER);
                DrawTextShadow(ops[i], tx+20, ty+6, 28, col);
            }

            DrawTextShadow("Use W/S ou UP/DOWN para navegar, ENTER para confirmar", x+28, y+h-48, 14, LIGHTGRAY);
            break;
        }


        // ============================================================
        // RANKING
        // ============================================================
        case TELA_RANKING:
        {
            DrawRectangle(0,0,sw,sh, Fade(BLACK,0.35f));
            int w = 520, h = 360, x = sw/2 - w/2, y = sh/2 - h/2;
            DrawRectangle(x-6,y-6,w+12,h+12, MIL_BORDER);
            DrawRectangle(x,y,w,h, MIL_DARK);

            DrawTextShadow("RANKING - TOP 5", x+24, y+16, 28, MIL_ACCENT);

            for (int i=0;i<MAX_RANK;i++)
            {
                Color col = LIGHTGRAY;
                DrawText(TextFormat("%d.", i+1), x+40, y+70 + i*50, 22, MIL_ACCENT);
                DrawText(ranking[i].nome, x+80, y+70 + i*50, 22, col);
                DrawText(TextFormat("%d", ranking[i].pontos), x+380, y+70 + i*50, 22, col);
            }

            DrawTextShadow("Pressione ENTER para voltar", x+24, y+h-48, 16, LIGHTGRAY);
            if(IsKeyPressed(KEY_ENTER)) telaAtual=TELA_MENU;
            break;
        }


        // ============================================================
        // JOGO (com explosões)
        // ============================================================
        case TELA_JOGO:
        {
            float dt=GetFrameTime();
            tempoScore+=dt;
            timerTiro += dt;

            // ATUALIZA EXPLOSOES
            for (int i=0;i<MAX_EXPLOSOES;i++){
                if(!explosoes[i].ativa) continue;
                explosoes[i].tempo += dt;
                float tnorm = explosoes[i].duracao > 0 ? (explosoes[i].tempo / explosoes[i].duracao) : 1.0f;
                int f = (int)(tnorm * explosoes[i].totalFrames);
                explosoes[i].frameAtual = f;
                if (explosoes[i].frameAtual >= explosoes[i].totalFrames) explosoes[i].ativa = false;
            }

            if(tempoScore>=1)
            {
                score+=10;
                tempoScore=0;
            }

            direcao=0;

            // Pause com ENTER
            if(IsKeyPressed(KEY_ENTER))
            {
                telaAtual=TELA_PAUSE;
                break;
            }

            // TIRO segurando espaço (cadência)
            if (IsKeyDown(KEY_SPACE) && timerTiro >= cadenciaTiro)
            {
                for (int i=0;i<MAX_BALAS;i++)
                {
                    if (!balas[i].ativa)
                    {
                        balas[i].ativa = true;
                        balas[i].w = 4;
                        balas[i].h = 10;
                        balas[i].x = px + TILE/2.0f - balas[i].w/2.0f;
                        balas[i].y = py - balas[i].h - 2;
                        break;
                    }
                }
                timerTiro = 0.0f;
            }

            // Movimento A/D
            if(IsKeyDown(KEY_A))
            {
                float nx=px-velocidadeLateral;
                if (nx < 0) nx = 0;
                if(vaiColidir(mapa,nx,py)) {
                    // cria explosão no avião (centro do avião)
                    float ex = px + TILE/2.0f - g_expl_frameW/2.0f;
                    float ey = py + TILE/2.0f - g_expl_spriteH/2.0f;
                    criarExplosao(explosoes, MAX_EXPLOSOES, ex, ey, 0.6f, g_expl_totalFrames);
                    telaAtual=TELA_FIM;
                    break;
                }
                else {px=nx; direcao=-1;}
            }
            else if(IsKeyDown(KEY_D))
            {
                float nx=px+velocidadeLateral;
                if (nx > (COLUNAS-1)*TILE) nx = (COLUNAS-1)*TILE;
                if(vaiColidir(mapa,nx,py)) {
                    float ex = px + TILE/2.0f - g_expl_frameW/2.0f;
                    float ey = py + TILE/2.0f - g_expl_spriteH/2.0f;
                    criarExplosao(explosoes, MAX_EXPLOSOES, ex, ey, 0.6f, g_expl_totalFrames);
                    telaAtual=TELA_FIM;
                    break;
                }
                else {px=nx; direcao=1;}
            }

            // W/S - frente/slow
            float velF=velocidadeFrente;
            if(IsKeyDown(KEY_S)) velF*=0.5f;
            if(IsKeyDown(KEY_W)) velF*=2;

            float ny=py-velF;
            if (ny < 0) ny = 0;
            if(vaiColidir(mapa,px,ny)) {
                float ex = px + TILE/2.0f - g_expl_frameW/2.0f;
                float ey = py + TILE/2.0f - g_expl_spriteH/2.0f;
                criarExplosao(explosoes, MAX_EXPLOSOES, ex, ey, 0.6f, g_expl_totalFrames);
                telaAtual=TELA_FIM;
                break;
            }
            else py=ny;

            // Combustível
            gasolina -= 5*dt;
            if(gasolina<=0)
            {
                gasolina=0;
                telaAtual=TELA_SEM_GASOLINA;
            }

            // Coletar F U E L (com checagem de limites)
            if(!fuelColetado)
            {
                int tx=(int)(px/TILE);
                int ty=(int)(py/TILE);

                if (tx>=0 && tx<COLUNAS && ty>=0 && ty<LINHAS)
                {
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

                    if (!carregarMapa(mapa,faseN)) {
                        for (int r=0;r<LINHAS;r++){
                            for (int c=0;c<COLUNAS;c++) mapa[r][c] = ' ';
                            mapa[r][COLUNAS] = '\0';
                        }
                    }
                    acharAviaoInicial(mapa,&ax,&ay);
                    px=ax*TILE;
                    py=ay*TILE;
                    fuelColetado=false;
                }
            }

            // Balas: mover e checar colisões
            for (int i=0;i<MAX_BALAS;i++)
            {
                if (!balas[i].ativa) continue;

                balas[i].y -= velocidadeBala * dt;

                if (balas[i].y + balas[i].h < 0)
                {
                    balas[i].ativa = false;
                    continue;
                }

                int bx_center = (int)((balas[i].x + balas[i].w/2) / TILE);
                int by_center = (int)((balas[i].y + balas[i].h/2) / TILE);

                if (bx_center >= 0 && bx_center < COLUNAS && by_center >= 0 && by_center < LINHAS)
                {
                    char tile = mapa[by_center][bx_center];
                    if (tile == 'N' || tile == 'X' || tile == 'V')
                    {
                        // cria explosão no centro do tile (ajusta para centralizar)
                        float ex = bx_center * TILE + (TILE - g_expl_frameW) / 2.0f;
                        float ey = by_center * TILE + (TILE - g_expl_spriteH) / 2.0f;
                        criarExplosao(explosoes, MAX_EXPLOSOES, ex, ey, 1.0f, g_expl_totalFrames);

                        mapa[by_center][bx_center] = ' ';
                        balas[i].ativa = false;
                        score += 200;
                    }
                }
            }

            // Render mapa
            desenharMapa(mapa, terra, agua, navio,
                         fuelF, fuelU, fuelE, fuelL,
                         helicoptero, ponte);

            Texture2D avi =
                (direcao==1)?aviao_direita :
                (direcao==-1)?aviao_esquerda :
                aviao_parado;

            DrawTexture(avi,(int)px,(int)py,WHITE);

            // Desenhar explosões (sobre mapa; mapa já desenha água por padrão para espaço)
            for (int i=0;i<MAX_EXPLOSOES;i++){
                if(!explosoes[i].ativa) continue;
                int f = explosoes[i].frameAtual;
                if (f < 0) f = 0;
                if (f >= explosoes[i].totalFrames) f = explosoes[i].totalFrames - 1;
                Rectangle src = { f * g_expl_frameW, 0, g_expl_frameW, g_expl_spriteH };
                Vector2 pos = { explosoes[i].x, explosoes[i].y };
                DrawTextureRec(explosaoSprite, src, pos, WHITE);
            }

            // Balas (sobre explosões)
            for (int i=0;i<MAX_BALAS;i++)
                if(balas[i].ativa)
                    DrawRectangle((int)balas[i].x,(int)balas[i].y, (int)balas[i].w, (int)balas[i].h, YELLOW);

            // HUD militar (barra + score)
            int hudX = 12;
            int hudY = 10;
            int hudW = 220, hudH = 26;
            DrawRectangle(hudX-4, hudY-4, hudW+8, hudH+8, MIL_BORDER);
            DrawRectangle(hudX, hudY, hudW, hudH, MIL_DARK);

            float prop = gasolina/100.0f;
            if (prop < 0) prop = 0;
            if (prop > 1) prop = 1;
            float filled = (hudW-6) * prop;
            DrawRectangle(hudX+3, hudY+3, hudW-6, hudH-6, (Color){16,16,16,255});
            DrawRectangle(hudX+3, hudY+3, (int)filled, hudH-6, MIL_OLIVE);
            DrawRectangleLines(hudX+3, hudY+3, hudW-6, hudH-6, MIL_BORDER);

            DrawTextShadowFormat(hudX + hudW + 8, hudY, 18, MIL_ACCENT, "%.0f%%", gasolina);
            DrawTextShadowFormat(10, hudY + hudH + 6, 20, MIL_ACCENT, "Score: %d", score);

            break;
        }


        // ============================================================
        // PAUSE
        // ============================================================
        case TELA_PAUSE:
        {
            DrawRectangle(0,0,sw,sh, Fade(BLACK,0.5f));

            int w = 520, h = 260, x = sw/2 - w/2, y = sh/2 - h/2;
            DrawRectangle(x-8,y-8,w+16,h+16, MIL_BORDER);
            DrawRectangle(x,y,w,h, MIL_DARK);

            DrawTextShadow("PAUSE", x+28, y+18, 36, MIL_ACCENT);

            static int opcPause = 0;
            const char* ops[] = {"Continuar", "Voltar ao menu"};

            for (int i=0;i<2;i++)
            {
                int bx = x + 40;
                int by = y + 80 + i*70;
                DrawRectangle(bx-10, by-6, 440, 52, (i==opcPause) ? (Color){24,34,24,255} : (Color){18,18,18,255});
                DrawRectangleLines(bx-10, by-6, 440, 52, MIL_BORDER);
                DrawTextShadow(ops[i], bx+18, by+8, 28, (i==opcPause)? MIL_ACCENT : LIGHTGRAY);
            }

            DrawTextShadow("Use W/S ou UP/DOWN para navegar. ENTER para confirmar.", x+28, y+h-48, 14, LIGHTGRAY);

            if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) opcPause = (opcPause + 1) % 2;
            if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) opcPause = (opcPause + 1) % 2;

            if(IsKeyPressed(KEY_ENTER))
            {
                if(opcPause==0) telaAtual = TELA_JOGO;
                else
                {
                    atualizarRanking(ranking,nome,score);
                    salvarRanking(ranking,"ranking.txt");
                    telaAtual = TELA_MENU;
                }
            }
            break;
        }


        // ============================================================
        // GAME OVER
        // ============================================================
        case TELA_FIM:
        {
            DrawRectangle(0,0,sw,sh, Fade(BLACK,0.45f));
            int w = 500, h = 220, x = sw/2 - w/2, y = sh/2 - h/2;
            DrawRectangle(x-6,y-6,w+12,h+12, MIL_BORDER);
            DrawRectangle(x,y,w,h, MIL_DARK);

            DrawTextShadow("BOOM! Fim de fase!", x+28, y+24, 28, RED);
            DrawTextShadowFormat(x+28, y+80, 22, LIGHTGRAY, "Score: %d", score);
            DrawTextShadow("Pressione ENTER para voltar ao menu", x+28, y+h-48, 16, LIGHTGRAY);

            if(IsKeyPressed(KEY_ENTER))
            {
                atualizarRanking(ranking,nome,score);
                salvarRanking(ranking,"ranking.txt");
                telaAtual=TELA_MENU;
            }
            break;
        }


        // ============================================================
        // VITORIA
        // ============================================================
        case TELA_VITORIA:
        {
            DrawRectangle(0,0,sw,sh, Fade(BLACK,0.45f));
            int ww = 540, hh = 240, xx = sw/2 - ww/2, yy = sh/2 - hh/2;
            DrawRectangle(xx-6,yy-6,ww+12,hh+12, MIL_BORDER);
            DrawRectangle(xx,yy,ww,hh, MIL_DARK);

            DrawTextShadow("PARABENS! TODAS AS FASES!", xx+28, yy+22, 28, GREEN);
            DrawTextShadowFormat(xx+28, yy+84, 22, LIGHTGRAY, "Score: %d", score);
            DrawTextShadow("Pressione ENTER para voltar", xx+28, yy+hh-48, 16, LIGHTGRAY);

            if(IsKeyPressed(KEY_ENTER))
            {
                atualizarRanking(ranking,nome,score);
                salvarRanking(ranking,"ranking.txt");
                telaAtual=TELA_MENU;
            }
            break;
        }


        // ============================================================
        // FIM GASOLINA
        // ============================================================
        case TELA_SEM_GASOLINA:
        {
            DrawRectangle(0,0,sw,sh, Fade(BLACK,0.45f));
            int w = 480, h = 200, x = sw/2 - w/2, y = sh/2 - h/2;
            DrawRectangle(x-6,y-6,w+12,h+12, MIL_BORDER);
            DrawRectangle(x,y,w,h, MIL_DARK);

            DrawTextShadow("ACABOU GASOLINA!", x+28, y+24, 28, ORANGE);
            DrawTextShadowFormat(x+28, y+84, 22, LIGHTGRAY, "Score: %d", score);
            DrawTextShadow("Pressione ENTER para voltar", x+28, y+h-48, 16, LIGHTGRAY);

            if(IsKeyPressed(KEY_ENTER))
            {
                atualizarRanking(ranking,nome,score);
                salvarRanking(ranking,"ranking.txt");
                telaAtual=TELA_MENU;
            }
            break;
        }


        // ============================================================
        // SAIR APP (ESC GLOBAL)
        // ============================================================
        case TELA_SAIR_APP:
        {
            DrawRectangle(0,0,sw,sh, Fade(BLACK,0.6f));
            int w = 420, h = 180, x = sw/2 - w/2, y = sh/2 - h/2;
            DrawRectangle(x-6,y-6,w+12,h+12, MIL_BORDER);
            DrawRectangle(x,y,w,h, MIL_DARK);

            DrawTextShadow("Deseja realmente sair?", x+28, y+18, 22, MIL_ACCENT);

            static int opcExit = 0;
            const char* opsExit[] = {"NAO","SIM"};

            for (int i=0;i<2;i++)
            {
                int bx = x + 40;
                int by = y + 60 + i*56;
                DrawRectangle(bx-6, by-6, 320, 48, (i==opcExit) ? (Color){28,36,28,255} : (Color){18,18,18,255});
                DrawRectangleLines(bx-6, by-6, 320, 48, MIL_BORDER);
                DrawTextShadow(opsExit[i], bx+120, by+8, 26, (i==opcExit)? MIL_ACCENT : LIGHTGRAY);
            }



            if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) opcExit = (opcExit + 1) % 2;
            if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) opcExit = (opcExit + 1) % 2;

            if(IsKeyPressed(KEY_ENTER))
            {
                if(opcExit==0) telaAtual = telaAnterior;
                else CloseWindow();
            }
            break;
        }

        } // fim switch

        EndDrawing();
    } // fim loop

    // Unload
    UnloadTexture(ponte);
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
    UnloadTexture(explosaoSprite);

    CloseWindow();
    return 0;
}
