#include <raylib.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>


// Constants
#define MAX_GRID_SIZE 8
#define CELL_SIZE 90
#define PANEL_WIDTH 260
#define COLOR_COUNT 5
#define MAX_MOVES 30
#define LEVEL_UP_DURATION 3.0f
#define MAX_SELECTED 100
#define MAX_PARTICLES 100
#define MAX_LEVELS 10
#define TRANSITION_DURATION 1.0f
#define MAX_OBSTACLES 20
#define EMPTY_CELL -1     
#define OBSTACLE_CELL -2  

// Global Variables
int grid[MAX_GRID_SIZE][MAX_GRID_SIZE];
int gridSize = 6;
int score = 0;
int movesLeft = MAX_MOVES;
int highScore = 0;
bool gameOver = false;
bool levelUp = false;
bool showLevelUpScreen = false;
bool loopDetected = false;
float volume = 0.5f;  // Başlangıç sesi
float levelUpTimer = 0.0f;
bool showhowtoplay = false;
bool showSettings = false;
bool soundOn = true;
float startScreenTimer = 0.0f;
bool musicOn = true;
float transitionTimer = 0.0f;
bool inTransition = false;
bool specialPowerActive = false;
bool specialPowerUsed = false;
int swapCount = 0;;
bool rowSwapPowerUsed = false;
int selectedRows[2] = { -1, -1 };
int selectedRowCount = 0;
bool rowSwapPowerActive = false;
bool showSwapRowsMessage = false;
float swapRowsMessageTimer = 0.0f;
bool showRowSwapClickText = false;
float rowSwapClickTextTimer = 0.0f;

// Game State
typedef enum GameState {
    MENU,
    LEVEL_SELECT,
    PLAYING,
    LEVEL_COMPLETE,
    SETTINGS_MENU  // Yeni eklenen
} GameState;
GameState gameState = MENU;
int currentLevelNumber = 1;

// Sound Variables
Sound clickSound;
Sound popSound;
Sound levelCompleteSound;
Music backgroundMusic;
Music music;

// Color Palette (Two Dots-style pastel colors) 

Color colors[COLOR_COUNT];
void InitColors() {
    colors[0] = (Color){ 255, 0, 0, 255 };      // Kırmızı
    colors[1] = (Color){ 0, 255, 0, 255 };      // Yeşil
    colors[2] = (Color){ 0, 0, 255, 255 };      // Mavi
    colors[3] = (Color){ 255, 255, 0, 255 };    // Sarı
    colors[4] = (Color){ 255, 128, 0, 255 };    // Turuncu
}
typedef struct {
    int x, y;
} Obstacle;

// Particle System
typedef struct {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float life;
    float size;
} Particle;

Particle particles[MAX_PARTICLES];
int particleCount = 0;

// Structures
typedef struct {
    int x;
    int y;
} Cell;

typedef struct {
    float scale;
    float timer;
    float offsetY; // For falling animation
} CellAnimation;

typedef struct {
    int level;
    int gridSize;
    int moves;
    int targetCount[COLOR_COUNT];
    Obstacle obstacles[MAX_OBSTACLES];
    int obstacleCount;


} Level;

typedef struct {
    int x;
    int y;
    bool active;
} Dot;

// Global Arrays
Vector2 selectedSwapCells[2];
Cell selectedCells[MAX_SELECTED];
int selectedCount = 0;
Level levels[MAX_LEVELS];
Dot backgroundDots[20];
CellAnimation cellAnimations[MAX_GRID_SIZE][MAX_GRID_SIZE];

// Computed Constants
#define GRID_WIDTH (gridSize * CELL_SIZE)
#define SCREEN_WIDTH (GRID_WIDTH + PANEL_WIDTH)
#define SCREEN_HEIGHT (gridSize * CELL_SIZE)

// Function Prototypes
void InitGame();
Level InitLevel(int level);
void InitGrid();
void UpdateSelection(int mouseX, int mouseY);
bool IsAlreadySelected(int x, int y);
bool AreAdjacent(Cell a, Cell b);
void ClearSelected();
void ApplyGravity();
void DrawMainMenu();
void DrawLevelSelect();
void DrawLevelComplete();
void InitParticles();
void UpdateParticles();
void DrawParticles();
short* GenerateSineWaveData(float frequency, float duration, int sampleRate);
float EaseInOutQuad(float t);
float EaseOutBounce(float t);
void ClearColor(int colorIndex);
void UpdateWindowSize();

// Update window size safely
void UpdateWindowSize() {
    if (GetScreenWidth() != SCREEN_WIDTH || GetScreenHeight() != SCREEN_HEIGHT) {
        SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
        printf("Window size updated to %dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    }
}

int main() {
    InitAudioDevice();

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Two Dots");
    SetTargetFPS(60);
    srand(time(NULL));
    Texture2D super1 = LoadTexture("super2.jpg");
    Texture2D super2 = LoadTexture("super1.jpg");
    Music music = LoadMusicStream("soundtrack.wav"); // muzik dosyasi
    PlayMusicStream(music);  
    bool musicPlaying = true;

    // Load Sounds
    clickSound = LoadSound("clicksound.mp3"); // clicksound sesi
    if (clickSound.frameCount == 0) {
        printf("Hata: clicksound.mp3 yuklenemedi! Dosya yolunu kontrol edin.\n");
    }
    popSound = LoadSound("pop.mp3"); // patlatma sesi
    if (popSound.frameCount == 0) {
        printf("Hata: pop.mp3 yuklenemedi! Dosya yolunu kontrol edin.\n");
    }
    levelCompleteSound = LoadSound("levelcomplete.mp3"); // level tamamlama sesi
    if (levelCompleteSound.frameCount == 0) {
        printf("Hata: levelcomplete.mp3 yuklenemedi! Dosya yolunu kontrol edin.\n");
    }

    // background muzigi sinus dalgariyla
    short* musicData = GenerateSineWaveData(220.0f, 10.0f, 44100);
    backgroundMusic = LoadMusicStreamFromMemory(".raw", musicData, 44100 * 10.0f * 2);
    free(musicData);
    SetMusicVolume(backgroundMusic, 0.3f);
    PlayMusicStream(backgroundMusic);

    InitGame();
    InitColors();
    while (!WindowShouldClose()) {
        UpdateMusicStream(music);
        UpdateMusicStream(backgroundMusic);
        startScreenTimer += GetFrameTime();

        // Handle Transitions
        if (inTransition) {
            transitionTimer -= GetFrameTime();
            if (transitionTimer <= 0.0f) inTransition = false;
            BeginDrawing();
            ClearBackground(RAYWHITE);
            float alpha = transitionTimer / TRANSITION_DURATION;
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, alpha));
            EndDrawing();
            continue;
        }

        // Update Game State
        switch (gameState) {
        case MENU:
            DrawMainMenu();
            if (showSettings == true){
                Rectangle buttonBounds = { SCREEN_WIDTH / 2 - 130, SCREEN_HEIGHT / 2 - 50, 260, 35 }; //muzik butonu
                if (CheckCollisionPointRec(GetMousePosition(), buttonBounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                {
                    if (musicPlaying)
                    {
                        PauseMusicStream(music);
                        musicPlaying = false;
                    }
                    else
                    {
                        PlayMusicStream(music);
                        musicPlaying = true;
                    }
                }
            }
            break;
        case LEVEL_SELECT:
            DrawLevelSelect();
            break;
        case PLAYING:
            if (levelUpTimer > 0.0f) {
                levelUpTimer -= GetFrameTime();
                UpdateParticles();
                if (levelUpTimer <= 0.0f) levelUp = false;
            }
            if (showLevelUpScreen) {
                DrawLevelComplete();
            }
            else {
                if (gameOver && IsKeyPressed(KEY_R)) {
                    score = 0;
                    movesLeft = levels[currentLevelNumber - 1].moves;
                    selectedCount = 0;
                    gameOver = false;
                    gameState = MENU;
                    InitGrid();
                    if (soundOn) PlaySound(clickSound);
                    specialPowerUsed = false;
                    specialPowerActive = false;
                    swapCount = 0;
                }
                if (!gameOver && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                    UpdateSelection(GetMouseX(), GetMouseY());
                }
                if (!gameOver && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    if (selectedCount >= 2) {
                        if (loopDetected) {
                            int color = grid[selectedCells[0].y][selectedCells[0].x];
                            ClearColor(color);
                            PlaySound(popSound);
                        }
                        else {
                            ClearSelected();
                            ApplyGravity();
                            PlaySound(popSound);
                        }
                    }
                    selectedCount = 0;
                    loopDetected = false;
                }
                if (!specialPowerUsed && IsKeyPressed(KEY_V)) {
                    specialPowerActive = true;
                    swapCount = 0; // Önceki seçimleri temizle
                }
                if (!gameOver && specialPowerActive && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    int cellX = GetMouseX() / CELL_SIZE;
                    int cellY = GetMouseY() / CELL_SIZE;

                    // Hücrenin geçerli ve engel olmadığını kontrol et
                    if (cellX >= 0 && cellX < gridSize && cellY >= 0 && cellY < gridSize && grid[cellY][cellX] != OBSTACLE_CELL) {
                        selectedSwapCells[swapCount].x = cellX;
                        selectedSwapCells[swapCount].y = cellY;
                        swapCount++;

                        // İki hücre seçildiyse, ikisinin de engel olmadığını kontrol et
                        if (swapCount == 2) {
                            if (grid[(int)selectedSwapCells[0].y][(int)selectedSwapCells[0].x] != OBSTACLE_CELL &&
                                grid[(int)selectedSwapCells[1].y][(int)selectedSwapCells[1].x] != OBSTACLE_CELL) {
                                // Yer değiştirme işlemini yap
                                int temp = grid[(int)selectedSwapCells[0].y][(int)selectedSwapCells[0].x];
                                grid[(int)selectedSwapCells[0].y][(int)selectedSwapCells[0].x] = grid[(int)selectedSwapCells[1].y][(int)selectedSwapCells[1].x];
                                grid[(int)selectedSwapCells[1].y][(int)selectedSwapCells[1].x] = temp;

                                // Ses çal ve bayrakları güncelle
                                if (soundOn) PlaySound(popSound);
                                specialPowerUsed = true;
                                specialPowerActive = false;
                                swapCount = 0;

                                // Yer değiştiren hücreler için animasyon başlat
                                cellAnimations[(int)selectedSwapCells[0].y][(int)selectedSwapCells[0].x].timer = 0.5f;
                                cellAnimations[(int)selectedSwapCells[0].y][(int)selectedSwapCells[0].x].offsetY = 0;
                                cellAnimations[(int)selectedSwapCells[1].y][(int)selectedSwapCells[1].x].timer = 0.5f;
                                cellAnimations[(int)selectedSwapCells[1].y][(int)selectedSwapCells[1].x].offsetY = 0;
                            }
                            else {
                                // Engel seçildiyse seçimi sıfırla
                                swapCount = 0;
                                if (soundOn) PlaySound(clickSound); 
                            }
                        }
                    }
                }
                // Update animations
                for (int y = 0; y < gridSize; y++) {
                    for (int x = 0; x < gridSize; x++) {
                        if (cellAnimations[y][x].timer > 0) {
                            cellAnimations[y][x].timer -= GetFrameTime();
                            cellAnimations[y][x].scale = 1.0f + 0.2f * EaseOutBounce(cellAnimations[y][x].timer / 0.5f);
                            cellAnimations[y][x].offsetY = -CELL_SIZE * (1.0f - EaseInOutQuad(cellAnimations[y][x].timer / 0.5f));
                            if (cellAnimations[y][x].timer <= 0) {
                                cellAnimations[y][x].scale = 1.0f;
                                cellAnimations[y][x].timer = 0;
                                cellAnimations[y][x].offsetY = 0;
                            }
                        }
                    }
                }
            }
            break;
        case LEVEL_COMPLETE:
            DrawLevelComplete();
            break;
        }

        // Draw Game
        if (gameState == PLAYING && !showLevelUpScreen) {
            BeginDrawing();

            ClearBackground((Color) { 245, 245, 245, 255 }); // Two Dots-style light background

            for (int y = 0; y < gridSize; y++) {
                for (int x = 0; x < gridSize; x++) {
                    int colorIndex = grid[y][x];
                    if (colorIndex != -1) {
                        int centerX = x * CELL_SIZE + CELL_SIZE / 2;
                        int centerY = y * CELL_SIZE + CELL_SIZE / 2 + cellAnimations[y][x].offsetY;
                        int radius = (CELL_SIZE - 10) / 2;
                        float scale = cellAnimations[y][x].scale;
                        DrawCircle(centerX, centerY, radius * scale, colors[colorIndex]);
                        DrawCircleLines(centerX, centerY, radius * scale, Fade(DARKGRAY, 0.3f)); // Subtle outline



                    }
                }
            }
            int panelvisualX = SCREEN_WIDTH - 180;
            int panelvisualY = 290;
            int panelvisualWidth = 170;
            int panelvisualHeight = 100;
            DrawRectangle(panelvisualX, panelvisualY, panelvisualWidth, panelvisualHeight, Fade(GRAY, 0.15f));
            Rectangle leftHalf = {
             panelvisualX,
             panelvisualY,
             panelvisualWidth / 2,
             panelvisualHeight
            };
            Rectangle rightHalf = {
            panelvisualX + panelvisualWidth / 2,
            panelvisualY,
            panelvisualWidth / 2,
            panelvisualHeight
            };
            Vector2 mousePos = GetMousePosition();

            if (CheckCollisionPointRec(GetMousePosition(), leftHalf) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (!rowSwapPowerActive && !rowSwapPowerUsed) {
                    rowSwapPowerActive = true;
                    selectedRowCount = 0; // önceki seçimleri temizle
                }
            }

            if (CheckCollisionPointRec(GetMousePosition(), rightHalf) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (!specialPowerActive && !specialPowerUsed) {
                    specialPowerActive = true;
                    swapCount = 0; // önceki seçimleri temizle
                }
            }




            for (int i = 0; i < selectedCount - 1; i++) {
                int x1 = selectedCells[i].x * CELL_SIZE + CELL_SIZE / 2;
                int y1 = selectedCells[i].y * CELL_SIZE + CELL_SIZE / 2;
                int x2 = selectedCells[i + 1].x * CELL_SIZE + CELL_SIZE / 2;
                int y2 = selectedCells[i + 1].y * CELL_SIZE + CELL_SIZE / 2;
                DrawLineEx((Vector2) { x1, y1 }, (Vector2) { x2, y2 }, 6.0f, Fade(colors[grid[selectedCells[i].y][selectedCells[i].x]], 0.8f));
            }

            for (int i = 0; i < selectedCount; i++) {
                int x = selectedCells[i].x;
                int y = selectedCells[i].y;
                int centerX = x * CELL_SIZE + CELL_SIZE / 2;
                int centerY = y * CELL_SIZE + CELL_SIZE / 2;
                int radius = (CELL_SIZE - 10) / 2;
                DrawCircleLines(centerX, centerY, radius + 3, Fade(WHITE, 0.5f));
            }

            DrawText(TextFormat("Skor: %d", score), 10, 10, 20, DARKGRAY);
            DrawText(TextFormat("Kalan Hamle: %d", movesLeft), 10, 40, 20, DARKGRAY);
            DrawText(TextFormat("Seviye: %d", currentLevelNumber), SCREEN_WIDTH - 160, 40, 20, DARKGRAY);

            DrawRectangle(SCREEN_WIDTH - 180, 70, 170, 200, Fade(GRAY, 0.2f));
            DrawRectangle(SCREEN_WIDTH - 180, 290, 170, 100, Fade(GRAY, 0.15f));
            DrawText("Hedefler:", SCREEN_WIDTH - 170, 80, 20, DARKGRAY);
            int offset = 0;
            for (int i = 0; i < COLOR_COUNT; i++) {
                if (levels[currentLevelNumber - 1].targetCount[i] > 0) {
                    DrawCircle(SCREEN_WIDTH - 50, 110 + offset * 30, 15, colors[i]);
                    DrawText(TextFormat("%d", levels[currentLevelNumber - 1].targetCount[i]), SCREEN_WIDTH - 100, 110 + offset * 30, 20, DARKGRAY);
                    offset++;
                }
            }
            Rectangle backToMenuButton = { SCREEN_WIDTH - 180, 450, 170, 40 };
            Vector2 mouse = GetMousePosition();
            DrawRectangleRounded(backToMenuButton, 0.2f, 10, CheckCollisionPointRec(mouse, backToMenuButton) ? (Color) { 200, 200, 200, 255 } : (Color) { 220, 220, 220, 255 });
            DrawText("Ana Menuye Don", backToMenuButton.x + 10, backToMenuButton.y + 10, 19, (Color) { 80, 80, 80, 255 });

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, backToMenuButton)) {
                gameState = MENU;
                score = 0;
                movesLeft = MAX_MOVES;
                selectedCount = 0;
                gameOver = false;
                specialPowerUsed = false;
                specialPowerActive = false;
                rowSwapPowerUsed = false;
                rowSwapPowerActive = false;
                InitGrid();
                transitionTimer = TRANSITION_DURATION;
                inTransition = true;
                if (soundOn) PlaySound(clickSound);
            }

            if (levelUp && levelUpTimer > 0.0f) {
                float t = 1.0f - (levelUpTimer / LEVEL_UP_DURATION);
                float alpha = EaseInOutQuad(t);
                DrawText("SEVİYE TAMAMLANDI!", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 100, 40, Fade(GRAY, alpha));
                DrawParticles();
            }
            if (gameOver) {
                DrawText("OYUN BİTTİ!", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 40, 40, MAROON);
                DrawText(TextFormat("En Yüksek Skor: %d", highScore), SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 30, 20, DARKGRAY);
                DrawText("Ana menü için R tuşuna basın", SCREEN_WIDTH / 2 - 110, SCREEN_HEIGHT / 2 + 10, 20, DARKGRAY);
            }
            if (specialPowerActive) {
                for (int i = 0; i < swapCount; i++) {
                    int x = selectedSwapCells[i].x;
                    int y = selectedSwapCells[i].y;
                    int centerX = x * CELL_SIZE + CELL_SIZE / 2;
                    int centerY = y * CELL_SIZE + CELL_SIZE / 2;
                    DrawCircleLines(centerX, centerY, CELL_SIZE / 2 + 5, Fade(VIOLET, 0.6f));
                }
                DrawText("Ozel Guc Kullanmak Icin 2 Farkli Top sec", 10, SCREEN_HEIGHT - 30, 20, VIOLET);
            }


            if (rowSwapPowerActive && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                int y = GetMouseY() / CELL_SIZE;



            }
            if (IsKeyPressed(KEY_B) && !rowSwapPowerUsed && !rowSwapPowerActive) {
                rowSwapPowerActive = true;
                selectedRowCount = 0;
                selectedRows[0] = -1;
                selectedRows[1] = -1;
            }
            if (rowSwapPowerActive) {
                int mouseX = GetMouseX();
                int mouseY = GetMouseY();

                // Tıklama grid alanındaysa
                if (mouseX >= 0 && mouseX < gridSize * CELL_SIZE &&
                    mouseY >= 0 && mouseY < gridSize * CELL_SIZE &&
                    IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {

                    int y = mouseY / CELL_SIZE;

                    // Aynı satır tekrar seçilmesin
                    bool alreadySelected = false;
                    for (int i = 0; i < selectedRowCount; i++) {
                        if (selectedRows[i] == y) {
                            alreadySelected = true;
                            break;
                        }
                    }

                    if (!alreadySelected && selectedRowCount < 2) {
                        selectedRows[selectedRowCount] = y;
                        selectedRowCount++;

                        // Yazıyı göster ve sayaç başlat
                        showRowSwapClickText = true;
                        rowSwapClickTextTimer = 2.0f;
                    }
                }

                // İki satır seçildiyse takasla
                if (selectedRowCount == 2) {
                    for (int x = 0; x < gridSize; x++) {
                        // Eğer herhangi biri engelse, bu sütunu atla
                        if (grid[selectedRows[0]][x] == OBSTACLE_CELL || grid[selectedRows[1]][x] == OBSTACLE_CELL) {
                            continue;
                        }

                        // Takas işlemi
                        int temp = grid[selectedRows[0]][x];
                        grid[selectedRows[0]][x] = grid[selectedRows[1]][x];
                        grid[selectedRows[1]][x] = temp;

                        // Animasyonlar
                        cellAnimations[selectedRows[0]][x].timer = 0.5f;
                        cellAnimations[selectedRows[1]][x].timer = 0.5f;

                        // İstersen offsetY de ekleyebilirsin
                        cellAnimations[selectedRows[0]][x].offsetY = (selectedRows[1] - selectedRows[0]) * CELL_SIZE;
                        cellAnimations[selectedRows[1]][x].offsetY = (selectedRows[0] - selectedRows[1]) * CELL_SIZE;
                    }


                    rowSwapPowerUsed = true;
                    rowSwapPowerActive = false;
                    selectedRowCount = 0;

                    // Bilgi mesajı göster
                    showSwapRowsMessage = true;
                    swapRowsMessageTimer = 3.0f;
                }

                // Seçilen satırları vurgula
                for (int i = 0; i < selectedRowCount; i++) {
                    int y = selectedRows[i];
                    DrawRectangle(0, y * CELL_SIZE, SCREEN_WIDTH, CELL_SIZE, Fade(ORANGE, 0.3f));
                }

                // Yazıyı sadece kısa süreli göster
                if (showRowSwapClickText) {
                    DrawText("2 Farkli Satir Sec: Row Swap Aktif", 10, SCREEN_HEIGHT - 60, 20, BLACK);
                    rowSwapClickTextTimer -= GetFrameTime();
                    if (rowSwapClickTextTimer <= 0) {
                        showRowSwapClickText = false;
                    }
                }
            }


            // Görsel efekt ve metin çizimi
            for (int i = 0; i < selectedRowCount; i++) {
                int y = selectedRows[i];
                DrawRectangle(0, y * CELL_SIZE, SCREEN_WIDTH, CELL_SIZE, Fade(ORANGE, 0.3f));
            }




            if (IsFileDropped()) {
                FilePathList droppedFiles = LoadDroppedFiles();
                printf("Dropped: %s\n", droppedFiles.paths[0]);
                UnloadDroppedFiles(droppedFiles);
            }
            // Panel çizimi
            // Panel boyutları
            int panelX = SCREEN_WIDTH - 180;
            int panelY = 290;
            int panelWidth = 170;
            int panelHeight = 100;

            // Resim arası boşluk
            int spacing = 10;

            // Kaç resim? 2
            int maxImages = 2;

            // Mevcut resim boyutu
            int imgWidth = super1.width;
            int imgHeight = super1.height;

            // Hedef resim yüksekliği: panelin içine sığacak şekilde scale hesapla
            float maxImageWidth = (panelWidth - spacing) / 2.0f;
            float maxImageHeight = panelHeight * 0.8f; // biraz boşluk kalsın

            // Her iki boyuta göre en küçük scale'i seç
            float scaleX = maxImageWidth / imgWidth;
            float scaleY = maxImageHeight / imgHeight;
            float finalScale = fminf(scaleX, scaleY);

            // Yeni boyutlar
            int scaledWidth = imgWidth * finalScale;
            int scaledHeight = imgHeight * finalScale;

            // Konumlar (ortalamak için hesaplanmış)
            int totalWidth = scaledWidth * 2 + spacing;
            int startX = panelX + (panelWidth - totalWidth) / 2;
            int imageY = panelY + (panelHeight - scaledHeight) / 2;

            // Çizim
            Vector2 pos1 = { startX, imageY };
            Vector2 pos2 = { startX + scaledWidth + spacing, imageY };

            DrawTextureEx(super1, pos2, 0.0f, finalScale, WHITE);
            DrawTextureEx(super2, pos1, 0.0f, finalScale, WHITE);


            EndDrawing();



        }
    }
    UnloadMusicStream(music);     // Bellekten sil

    UnloadSound(clickSound);
    UnloadSound(popSound);
    UnloadSound(levelCompleteSound);
    UnloadMusicStream(backgroundMusic);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

void InitGame() {
    for (int i = 0; i < MAX_LEVELS; i++) {
        levels[i] = InitLevel(i + 1);
    }
    for (int i = 0; i < 20; i++) {
        backgroundDots[i].x = GetRandomValue(0, SCREEN_WIDTH);
        backgroundDots[i].y = GetRandomValue(0, SCREEN_HEIGHT);
        backgroundDots[i].active = true;
    }
    gameState = MENU;
    currentLevelNumber = 1;
    score = 0;
    movesLeft = MAX_MOVES;
    gameOver = false;
    selectedCount = 0;
    InitGrid();
    UpdateWindowSize(); // Set initial window size
}

Level InitLevel(int level) {
    Level newLevel = { 0 };
    newLevel.level = level;
    newLevel.gridSize = (level <= 2) ? 6 : (level <= 4) ? 7 : 8; // Seviyeye göre grid boyutu
    newLevel.moves = 25 + level * 5;
    for (int i = 0; i < COLOR_COUNT; i++) {
        newLevel.targetCount[i] = (i % 2 == 0) ? 8 + level * 2 : 0; // İki renk hedefi
    }
    if (level >= 3) {
        int extraColor = GetRandomValue(0, COLOR_COUNT - 1);
        newLevel.targetCount[extraColor] = 10 + level * 2;
    }

    if (level == 2) {
        newLevel.obstacleCount = 4;
        newLevel.obstacles[0] = (Obstacle){ 2, 2 };
        newLevel.obstacles[1] = (Obstacle){ 2, 3 };
        newLevel.obstacles[2] = (Obstacle){ 3, 2 };
        newLevel.obstacles[3] = (Obstacle){ 3, 3 };
    }
    if (level == 3) {
        newLevel.obstacleCount = 10;
        newLevel.obstacles[0] = (Obstacle){ 2, 0 };
        newLevel.obstacles[1] = (Obstacle){ 3, 0 };
        newLevel.obstacles[2] = (Obstacle){ 4, 0 };
        newLevel.obstacles[3] = (Obstacle){ 6, 2 };
        newLevel.obstacles[4] = (Obstacle){ 0, 4 };
        newLevel.obstacles[5] = (Obstacle){ 2, 4 };
        newLevel.obstacles[6] = (Obstacle){ 3, 4 };
        newLevel.obstacles[7] = (Obstacle){ 4, 4 };
        newLevel.obstacles[8] = (Obstacle){ 6, 4 };
        newLevel.obstacles[9] = (Obstacle){ 6, 3 };
    }
    if (level == 4) {
        newLevel.obstacleCount = 7;
        newLevel.obstacles[0] = (Obstacle){ 0, 0 };
        newLevel.obstacles[1] = (Obstacle){ 1, 1 };
        newLevel.obstacles[2] = (Obstacle){ 2, 2 };
        newLevel.obstacles[3] = (Obstacle){ 3, 3 };
        newLevel.obstacles[4] = (Obstacle){ 4, 4 };
        newLevel.obstacles[5] = (Obstacle){ 5, 5 };
        newLevel.obstacles[6] = (Obstacle){ 6,6 };
    }
    if (level == 5) {
        newLevel.obstacleCount = 14;
        newLevel.obstacles[0] = (Obstacle){ 0, 0 };
        newLevel.obstacles[1] = (Obstacle){ 7, 0 };
        newLevel.obstacles[2] = (Obstacle){ 0, 5 };
        newLevel.obstacles[3] = (Obstacle){ 0, 6 };                             //sütun satır şeklinde yazılıyor <3
        newLevel.obstacles[4] = (Obstacle){ 0, 7 };
        newLevel.obstacles[5] = (Obstacle){ 1, 6 };
        newLevel.obstacles[6] = (Obstacle){ 1, 7 };
        newLevel.obstacles[7] = (Obstacle){ 2, 7 };
        newLevel.obstacles[8] = (Obstacle){ 5, 7 };
        newLevel.obstacles[9] = (Obstacle){ 6, 7 };
        newLevel.obstacles[10] = (Obstacle){ 7, 7 };
        newLevel.obstacles[11] = (Obstacle){ 6, 6 };
        newLevel.obstacles[12] = (Obstacle){ 7, 6 };
        newLevel.obstacles[13] = (Obstacle){ 7, 5 };


    }
    if (level == 6) {
        newLevel.obstacleCount = 12;
        newLevel.obstacles[0] = (Obstacle){ 2, 2 };
        newLevel.obstacles[1] = (Obstacle){ 3, 1 };
        newLevel.obstacles[2] = (Obstacle){ 4, 1 };
        newLevel.obstacles[3] = (Obstacle){ 5, 2 };
        newLevel.obstacles[4] = (Obstacle){ 6, 3 };
        newLevel.obstacles[5] = (Obstacle){ 1, 3 };
        newLevel.obstacles[6] = (Obstacle){ 1, 4 };
        newLevel.obstacles[7] = (Obstacle){ 2, 5 };
        newLevel.obstacles[8] = (Obstacle){ 3, 6 };
        newLevel.obstacles[9] = (Obstacle){ 4, 6 };
        newLevel.obstacles[10] = (Obstacle){ 5, 5 };
        newLevel.obstacles[11] = (Obstacle){ 6, 4 };
    }
    if (level == 7) {
        newLevel.obstacleCount = 16;
        newLevel.obstacles[0] = (Obstacle){ 0, 4 };
        newLevel.obstacles[1] = (Obstacle){ 0, 5 };
        newLevel.obstacles[2] = (Obstacle){ 0, 6 };
        newLevel.obstacles[3] = (Obstacle){ 0, 7 };
        newLevel.obstacles[4] = (Obstacle){ 1, 5 };
        newLevel.obstacles[5] = (Obstacle){ 1, 6 };
        newLevel.obstacles[6] = (Obstacle){ 2, 4 };
        newLevel.obstacles[7] = (Obstacle){ 2, 5 };
        newLevel.obstacles[8] = (Obstacle){ 3, 3 };
        newLevel.obstacles[9] = (Obstacle){ 3, 4 };
        newLevel.obstacles[10] = (Obstacle){ 4, 2 };
        newLevel.obstacles[11] = (Obstacle){ 4, 3 };
        newLevel.obstacles[12] = (Obstacle){ 5, 1 };
        newLevel.obstacles[13] = (Obstacle){ 5, 2 };
        newLevel.obstacles[14] = (Obstacle){ 6, 0 };
        newLevel.obstacles[15] = (Obstacle){ 6, 1 };
    }
    if (level == 8) {
        newLevel.obstacleCount = 14;
        newLevel.obstacles[0] = (Obstacle){ 0, 0 };
        newLevel.obstacles[1] = (Obstacle){ 1, 1 };
        newLevel.obstacles[2] = (Obstacle){ 2, 2 };
        newLevel.obstacles[3] = (Obstacle){ 3, 2 };
        newLevel.obstacles[4] = (Obstacle){ 4, 1 };
        newLevel.obstacles[5] = (Obstacle){ 6, 2 };
        newLevel.obstacles[6] = (Obstacle){ 7, 2 };
        newLevel.obstacles[7] = (Obstacle){ 0, 5 };
        newLevel.obstacles[8] = (Obstacle){ 1, 5 };
        newLevel.obstacles[9] = (Obstacle){ 3, 7 };
        newLevel.obstacles[10] = (Obstacle){ 4, 6 };
        newLevel.obstacles[11] = (Obstacle){ 5, 5 };
        newLevel.obstacles[12] = (Obstacle){ 6, 6 };
        newLevel.obstacles[13] = (Obstacle){ 7, 7 };

    }
    if (level == 9) {
        newLevel.obstacleCount = 12;
        newLevel.obstacles[0] = (Obstacle){ 3, 1 };
        newLevel.obstacles[1] = (Obstacle){ 3, 2 };
        newLevel.obstacles[2] = (Obstacle){ 3, 6 };
        newLevel.obstacles[3] = (Obstacle){ 3, 7 };
        newLevel.obstacles[4] = (Obstacle){ 1, 3 };
        newLevel.obstacles[5] = (Obstacle){ 2, 3 };
        newLevel.obstacles[6] = (Obstacle){ 4, 3 };
        newLevel.obstacles[7] = (Obstacle){ 5, 3 };
        newLevel.obstacles[8] = (Obstacle){ 1, 5 };
        newLevel.obstacles[9] = (Obstacle){ 2, 5 };
        newLevel.obstacles[10] = (Obstacle){ 4, 5 };
        newLevel.obstacles[11] = (Obstacle){ 5, 5 };


    }
    printf("Level %d: Grid %dx%d, Moves %d, Obstacles %d\n", level, newLevel.gridSize, newLevel.gridSize, newLevel.moves, newLevel.obstacleCount);
    return newLevel;
}

void InitGrid() {
    gridSize = levels[currentLevelNumber - 1].gridSize;
    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            grid[y][x] = GetRandomValue(0, COLOR_COUNT - 1);
            cellAnimations[y][x].scale = 0.0f;
            cellAnimations[y][x].timer = 0.5f + (y * gridSize + x) * 0.05f;
            cellAnimations[y][x].offsetY = -CELL_SIZE; // Start above grid
            printf("grid[%d][%d] = %d\n", y, x, grid[y][x]);
        }


    }
    Level currentLevel = levels[currentLevelNumber - 1];
    for (int i = 0; i < currentLevel.obstacleCount; i++) {
        int x = currentLevel.obstacles[i].x;
        int y = currentLevel.obstacles[i].y;
        if (x >= 0 && x < gridSize && y >= 0 && y < gridSize) {
            grid[y][x] = OBSTACLE_CELL;
        }
    }
    UpdateWindowSize(); // Update window size when grid changes
}

bool IsAlreadySelected(int x, int y) {
    for (int i = 0; i < selectedCount; i++) {
        if (selectedCells[i].x == x && selectedCells[i].y == y) return true;
    }
    return false;
}

bool AreAdjacent(Cell a, Cell b) {
    int dx = abs(a.x - b.x);
    int dy = abs(a.y - b.y);
    return (dx + dy == 1);
}

void UpdateSelection(int mouseX, int mouseY) {
    loopDetected = false;
    int x = mouseX / CELL_SIZE;
    int y = mouseY / CELL_SIZE;

    if (x < 0 || x >= gridSize || y < 0 || y >= gridSize) return;
    if (grid[y][x] == -1) return;
    if (grid[y][x] < 0) return;
    if (selectedCount > 0) {
        int lastX = selectedCells[selectedCount - 1].x;
        int lastY = selectedCells[selectedCount - 1].y;
        if (!AreAdjacent((Cell) { lastX, lastY }, (Cell) { x, y })) return;
        if (grid[y][x] != grid[lastY][lastX]) return;
    }

    if (IsAlreadySelected(x, y)) {
        if (selectedCount >= 4 && (x == selectedCells[0].x && y == selectedCells[0].y)) {
            loopDetected = true;
            printf("Loop detected! Color: %d\n", grid[y][x]);
        }
        return;
    }

    if (selectedCount < MAX_SELECTED) {
        selectedCells[selectedCount].x = x;
        selectedCells[selectedCount].y = y;
        cellAnimations[y][x].scale = 1.2f;
        cellAnimations[y][x].timer = 0.2f;
        selectedCount++;
    }
}

void ClearSelected() {
    int colorCount[COLOR_COUNT] = { 0 };
    int clearedCells = 0;

    for (int i = 0; i < selectedCount; i++) {
        int x = selectedCells[i].x;
        int y = selectedCells[i].y;
        int color = grid[y][x];
        colorCount[color]++;
        grid[y][x] = -1;
        clearedCells++;
        // Add particles for cleared dots
        for (int p = 0; p < 5; p++) {
            if (particleCount < MAX_PARTICLES) {
                particles[particleCount].position = (Vector2){ x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2 };
                float angle = GetRandomValue(0, 360) * DEG2RAD;
                float speed = GetRandomValue(50, 150);
                particles[particleCount].velocity = (Vector2){ cosf(angle) * speed, sinf(angle) * speed };
                particles[particleCount].color = colors[color];
                particles[particleCount].life = GetRandomValue(5, 15) / 10.0f;
                particles[particleCount].size = GetRandomValue(3, 10);
                particleCount++;
            }
        }
    }

    score += clearedCells * 10;
    if (score > highScore) highScore = score;
    movesLeft--;

    printf("Cleared cells: %d, Colors: ", clearedCells);
    for (int i = 0; i < COLOR_COUNT; i++) {
        printf("%d ", colorCount[i]);
    }
    printf("\n");

    bool levelComplete = true;
    for (int i = 0; i < COLOR_COUNT; i++) {
        levels[currentLevelNumber - 1].targetCount[i] -= colorCount[i];
        if (levels[currentLevelNumber - 1].targetCount[i] > 0) {
            levelComplete = false;
        }
        else {
            levels[currentLevelNumber - 1].targetCount[i] = 0;
        }
        printf("Target %d: %d remaining\n", i, levels[currentLevelNumber - 1].targetCount[i]);
    }

    if (levelComplete) {
        printf("Level %d completed!\n", currentLevelNumber);
        levelUp = true;
        showLevelUpScreen = true;
        levelUpTimer = LEVEL_UP_DURATION;
        InitParticles();
        PlaySound(levelCompleteSound);
        currentLevelNumber++;
        if (currentLevelNumber > MAX_LEVELS) currentLevelNumber = 1;
    }

    if (movesLeft <= 0 && !showLevelUpScreen) {
        gameOver = true;
        printf("Game over! No moves left.\n");
    }
    selectedCount = 0;
}

void ClearColor(int colorIndex) {
    int clearedCells = 0;
    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            if (grid[y][x] == colorIndex) {
                grid[y][x] = EMPTY_CELL;
                clearedCells++;

                // Partikül efekti
                for (int p = 0; p < 5; p++) {
                    if (particleCount < MAX_PARTICLES) {
                        particles[particleCount].position = (Vector2){ x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2 };
                        float angle = GetRandomValue(0, 360) * DEG2RAD;
                        float speed = GetRandomValue(50, 150);
                        particles[particleCount].velocity = (Vector2){ cosf(angle) * speed, sinf(angle) * speed };
                        particles[particleCount].color = colors[colorIndex];
                        particles[particleCount].life = GetRandomValue(5, 15) / 10.0f;
                        particles[particleCount].size = GetRandomValue(3, 10);
                        particleCount++;
                    }
                }
            }
        }
    }

    levels[currentLevelNumber - 1].targetCount[colorIndex] -= clearedCells;
    if (levels[currentLevelNumber - 1].targetCount[colorIndex] < 0) {
        levels[currentLevelNumber - 1].targetCount[colorIndex] = 0;
    }

    score += clearedCells * 50;
    if (score > highScore) highScore = score;
    movesLeft--;

    ApplyGravity();
}


void ApplyGravity() {
    for (int x = 0; x < gridSize; x++) {
        // Engel pozisyonlarını kaydet
        bool obstaclePositions[MAX_GRID_SIZE] = { false };
        for (int y = 0; y < gridSize; y++) {
            if (grid[y][x] == OBSTACLE_CELL) {
                obstaclePositions[y] = true;
            }
        }

        // Geçici dizi ile topları topla (engelleri atla)
        int tempGrid[MAX_GRID_SIZE];
        int tempIndex = 0;
        for (int y = 0; y < gridSize; y++) {
            if (grid[y][x] >= 0) { // Sadece topları al (0 veya pozitif)
                tempGrid[tempIndex] = grid[y][x];
                tempIndex++;
            }
        }

        // Sütunu en alttan yukarı doldur, engelleri sabit tut
        int writeY = gridSize - 1;
        int tempReadIndex = tempIndex - 1;
        for (int y = gridSize - 1; y >= 0; y--) {
            if (obstaclePositions[y]) {
                // Engel varsa, sabit bırak
                grid[y][x] = OBSTACLE_CELL;
                continue;
            }
            if (tempReadIndex >= 0) {
                // Top varsa, yaz ve animasyon başlat
                grid[y][x] = tempGrid[tempReadIndex];
                cellAnimations[y][x].scale = 0.0f;
                cellAnimations[y][x].timer = 0.5f;
                cellAnimations[y][x].offsetY = (tempReadIndex - y) * CELL_SIZE;
                tempReadIndex--;
            }
            else {
                // Boşlukları yeni toplarla doldur
                grid[y][x] = GetRandomValue(0, COLOR_COUNT - 1);
                cellAnimations[y][x].scale = 0.0f;
                cellAnimations[y][x].timer = 0.5f;
                cellAnimations[y][x].offsetY = -CELL_SIZE;
            }
        }
    }
}



void InitParticles() {
    particleCount = MAX_PARTICLES;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].position = (Vector2){ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(100, 300);
        particles[i].velocity = (Vector2){ cosf(angle) * speed, sinf(angle) * speed };
        particles[i].color = colors[GetRandomValue(0, COLOR_COUNT - 1)];
        particles[i].life = GetRandomValue(10, 30) / 10.0f;
        particles[i].size = GetRandomValue(5, 20);
    }
}

void UpdateParticles() {
    for (int i = 0; i < particleCount; i++) {
        particles[i].position.x += particles[i].velocity.x * GetFrameTime();
        particles[i].position.y += particles[i].velocity.y * GetFrameTime();
        particles[i].life -= GetFrameTime();
        if (particles[i].life <= 0) particles[i].size = 0;
        else particles[i].size = particles[i].size * EaseOutBounce(particles[i].life / particles[i].life);
    }
}

void DrawParticles() {
    for (int i = 0; i < particleCount; i++) {
        if (particles[i].life > 0) {
            DrawCircleV(particles[i].position, particles[i].size, Fade(particles[i].color, particles[i].life / 3.0f));
        }
    }
}

short* GenerateSineWaveData(float frequency, float duration, int sampleRate) {
    int sampleCount = (int)(duration * sampleRate);
    short* data = (short*)malloc(sampleCount * sizeof(short));
    if (!data) return NULL;

    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / sampleRate;
        data[i] = (short)(sinf(2.0f * PI * frequency * t) * 32760.0f * 0.5f);
    }

    return data;
}

void DrawMainMenu() {
    BeginDrawing();
    ClearBackground((Color) { 245, 245, 245, 255 });

    for (int i = 0; i < 20; i++) {
        if (backgroundDots[i].active) {
            backgroundDots[i].y += sinf(startScreenTimer + i) * 0.5f;
            if (backgroundDots[i].y > SCREEN_HEIGHT) backgroundDots[i].y -= SCREEN_HEIGHT;
            DrawCircle(backgroundDots[i].x, backgroundDots[i].y, 15, Fade(colors[i % COLOR_COUNT], 0.3f));
        }
    }

    float scale = 1.0f + 0.1f * sinf(startScreenTimer * 2.0f);
    int titleWidth = MeasureText("Two Dots", 60);
    DrawText("Two Dots", SCREEN_WIDTH / 2 - titleWidth / 2, SCREEN_HEIGHT / 4, 60 * scale, (Color) { 80, 80, 80, 255 });

    Rectangle playButton = { SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2, 200, 50 };
    Rectangle howToPlayButton = { SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 70, 200, 50 };
    Vector2 mouse = GetMousePosition();
    Rectangle settingsMenuButton = { SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 140, 200, 50 };
    DrawRectangleRounded(settingsMenuButton, 0.2f, 10, CheckCollisionPointRec(mouse, settingsMenuButton) ? (Color) { 200, 200, 200, 255 } : (Color) { 220, 220, 220, 255 });
    DrawText("Ayarlar", settingsMenuButton.x + 20, settingsMenuButton.y + 5, 20, (Color) { 80, 80, 80, 255 });

    DrawRectangleRounded(playButton, 0.2f, 10, CheckCollisionPointRec(mouse, playButton) ? (Color) { 200, 200, 200, 255 } : (Color) { 220, 220, 220, 255 });
    DrawText("Oyna", playButton.x + 70, playButton.y + 15, 20, (Color) { 80, 80, 80, 255 });
    DrawRectangleRounded(howToPlayButton, 0.2f, 10, CheckCollisionPointRec(mouse, howToPlayButton) ? (Color) { 200, 200, 200, 255 } : (Color) { 220, 220, 220, 255 });
    DrawText("Nasil Oynanir", howToPlayButton.x + 30, howToPlayButton.y + 15, 20, (Color) { 80, 80, 80, 255 });

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (CheckCollisionPointRec(mouse, playButton)) {
            gameState = LEVEL_SELECT;
            transitionTimer = TRANSITION_DURATION;
            inTransition = true;
            if (soundOn) PlaySound(clickSound);
        }
        else if (CheckCollisionPointRec(mouse, howToPlayButton)) {
            showhowtoplay = !showhowtoplay;
            if (soundOn) PlaySound(clickSound);
        }
        else if (CheckCollisionPointRec(mouse, settingsMenuButton)) {
            showSettings = !showSettings;
            if (soundOn) PlaySound(clickSound);
        }
    }

    if (showSettings) {
        Rectangle settingsBox = { SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 150, 300, 250 };
        DrawRectangleRounded(settingsBox, 0.2f, 10, (Color) { 230, 230, 230, 255 });
        DrawText("Ayarlar", settingsBox.x + 100, settingsBox.y + 10, 20, (Color) { 80, 80, 80, 255 });

        Rectangle closeButton = { settingsBox.x + settingsBox.width - 30, settingsBox.y + 10, 20, 20 };
        DrawRectangle(closeButton.x, closeButton.y, closeButton.width, closeButton.height, RED);
        DrawText("X", closeButton.x + 5, closeButton.y, 20, WHITE);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, closeButton)) {
            showSettings = false;
            if (soundOn) PlaySound(clickSound);
        }

        Rectangle soundToggle = { settingsBox.x + 20, settingsBox.y + 50, 260, 35 };
        Color soundToggleColor = soundOn ? (Color) { 100, 200, 100, 255 } : (Color) { 200, 100, 100, 255 };
        if (CheckCollisionPointRec(mouse, soundToggle)) {
            soundToggleColor = soundOn ? (Color) { 80, 180, 80, 255 } : (Color) { 180, 80, 80, 255 };
        }
        DrawRectangleRounded(soundToggle, 0.2f, 10, soundToggleColor);
        DrawText(soundOn ? "Ses Efektleri: Acik" : "Ses Efektleri: Kapali", soundToggle.x + 10, soundToggle.y + 5, 16, (Color) { 255, 255, 255, 255 });

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, soundToggle)) {
            soundOn = !soundOn;
            if (soundOn) PlaySound(clickSound);
        }

        Rectangle musicToggle = { settingsBox.x + 20, settingsBox.y + 100, 260, 35 };
        Color musicToggleColor = musicOn ? (Color) { 100, 200, 100, 255 } : (Color) { 200, 100, 100, 255 };
        if (CheckCollisionPointRec(mouse, musicToggle)) {
            musicToggleColor = musicOn ? (Color) { 80, 180, 80, 255 } : (Color) { 180, 80, 80, 255 };
        }
        DrawRectangleRounded(musicToggle, 0.2f, 10, musicToggleColor);
        DrawText(musicOn ? "Muzik: Acik" : "Muzik: Kapali", musicToggle.x + 10, musicToggle.y + 5, 16, (Color) { 255, 255, 255, 255 });

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, musicToggle)) {
            musicOn = !musicOn;
            if (!musicOn) {
                StopMusicStream(music);
            }
            else {
                PlayMusicStream(music);
            }
        }
    }

    if (showhowtoplay) {
        Rectangle howToPlayBox = { SCREEN_WIDTH / 2 - 300, SCREEN_HEIGHT / 2 - 100 - 150, 600, 450 };
        DrawRectangleRounded(howToPlayBox, 0.2f, 10, (Color) { 230, 230, 230, 255 });
        DrawText("Nasıl Oynanır?", howToPlayBox.x + 120, howToPlayBox.y + 20, 24, (Color) { 80, 80, 80, 255 });

        // Açıklama metinleri
        int textY = howToPlayBox.y + 50;
        DrawText("- Ayni renkli noktalari yatay ve dikey baglayin.", howToPlayBox.x + 20, textY, 20, (Color) { 80, 80, 80, 255 });
        DrawCircle(howToPlayBox.x + 560, textY + 8, 8, colors[0]); // Kırmızı nokta örneği
        DrawCircle(howToPlayBox.x + 580, textY + 8, 8, colors[0]);

        textY += 30;
        DrawText("- En az 2 nokta birlestirin, Kare olursaniz", howToPlayBox.x + 20, textY, 20, (Color) { 80, 80, 80, 255 });
        DrawText("  tum ayni renk noktalar silinecek.", howToPlayBox.x + 20, textY + 20, 20, (Color) { 80, 80, 80, 255 });

        textY += 45;
        DrawText("- Her seviyenin hedeflerini tamamlayin:", howToPlayBox.x + 20, textY + 40, 20, (Color) { 80, 80, 80, 255 });
        DrawCircle(howToPlayBox.x + 420, textY + 43, 8, colors[1]); // Yeşil nokta

        textY += 30;
        DrawText("- Ozel gucler kullanin:", howToPlayBox.x + 20, textY + 30, 20, (Color) { 80, 80, 80, 255 });
        DrawText("  * 'V' tusu: Iki noktayi yer degistirir.", howToPlayBox.x + 20, textY + 80, 20, (Color) { 80, 80, 80, 255 });
        DrawText("  * 'B' tusu: Iki satiri yer degistirir.", howToPlayBox.x + 20, textY + 100, 20, (Color) { 80, 80, 80, 255 });

        textY += 60;
        DrawText("- Hamleleriniz bitmeden hedeflerinizi tamamlayin!", howToPlayBox.x + 120, textY, 20, (Color) { 80, 80, 80, 255 });

        // Kapatma butonu
        Rectangle closeButton = { howToPlayBox.x + howToPlayBox.width - 30, howToPlayBox.y + 10, 20, 20 };
        DrawRectangle(closeButton.x, closeButton.y, closeButton.width, closeButton.height, RED);
        DrawText("X", closeButton.x + 5, closeButton.y, 20, WHITE);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, closeButton)) {
            showhowtoplay = false;
            if (soundOn) PlaySound(clickSound);
        }
    }

    EndDrawing();
}

void DrawLevelSelect() {
    BeginDrawing();
    ClearBackground((Color) { 245, 245, 245, 255 });

    for (int i = 0; i < 20; i++) {
        if (backgroundDots[i].active) {
            backgroundDots[i].y += sinf(startScreenTimer + i) * 0.5f;
            if (backgroundDots[i].y > SCREEN_HEIGHT) backgroundDots[i].y -= SCREEN_HEIGHT;
            DrawCircle(backgroundDots[i].x, backgroundDots[i].y, 15, Fade(colors[i % COLOR_COUNT], 0.3f));
        }
    }

    DrawText("Seviye Sec", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 6, 40, (Color) { 80, 80, 80, 255 });
    Vector2 mouse = GetMousePosition();

    // Seviye butonları
    for (int i = 0; i < MAX_LEVELS; i++) {
        Rectangle levelButton = { SCREEN_WIDTH / 2 - 200 + (i % 5) * 100, SCREEN_HEIGHT / 3 + (i / 5) * 100, 80, 80 };
        Color buttonColor = (i < currentLevelNumber) ? colors[i % COLOR_COUNT] : (Color) { 150, 150, 150, 255 };
        DrawCircle(levelButton.x + 40, levelButton.y + 40, 40, buttonColor);
        DrawText(TextFormat("%d", i + 1), levelButton.x + 30, levelButton.y + 30, 20, WHITE);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, levelButton) && i < currentLevelNumber) {
            currentLevelNumber = i + 1;
            gameState = PLAYING;
            movesLeft = levels[currentLevelNumber - 1].moves;
            gridSize = levels[currentLevelNumber - 1].gridSize;
            levels[currentLevelNumber - 1] = InitLevel(currentLevelNumber); // Seviye hedeflerini sıfırla
            InitGrid();
            transitionTimer = TRANSITION_DURATION;
            inTransition = true;
            if (soundOn) PlaySound(clickSound);
        }
    }

    // Veteran butonu (sol alta)
    Rectangle veteranButton = { 10, SCREEN_HEIGHT - 60, 200, 50 };
    bool isVeteranMode = (currentLevelNumber == MAX_LEVELS); // Veteran modu açık mı?
    DrawRectangleRounded(veteranButton, 0.2f, 10, CheckCollisionPointRec(mouse, veteranButton) ? (Color) { 200, 200, 200, 255 } : (Color) { 220, 220, 220, 255 });
    DrawText(isVeteranMode ? "Veteran: Kapali" : "Veteran: Acik", veteranButton.x + 30, veteranButton.y + 15, 20, (Color) { 80, 80, 80, 255 });

    // Veteran butonuna tıklama kontrolü (toggle mantığı)
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, veteranButton)) {
        if (isVeteranMode) {
            currentLevelNumber = 1; // Veteran modunu kapat, sadece 1. seviye açık
        }
        else {
            currentLevelNumber = MAX_LEVELS; // Veteran modunu aç, tüm seviyeler açık
        }
        if (soundOn) PlaySound(clickSound);
    }

    EndDrawing();
}

void DrawLevelComplete() {
    float t = 1.0f - (levelUpTimer / LEVEL_UP_DURATION);
    float alpha = EaseInOutQuad(t < 0.5f ? t * 2.0f : 1.0f - (t - 0.5f) * 2.0f);
    BeginDrawing();
    ClearBackground((Color) { 245, 245, 245, 255 });
    DrawParticles();
    DrawText(TextFormat("Seviye %d Tamamlandi!", currentLevelNumber - 1), SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 80, 40, Fade((Color) { 80, 80, 80, 255 }, alpha));
    DrawText("Devam icin ENTER'a basin", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 20, 20, Fade((Color) { 80, 80, 80, 255 }, alpha));
    EndDrawing();

    if (IsKeyPressed(KEY_ENTER)) {
        showLevelUpScreen = false;
        movesLeft = levels[currentLevelNumber - 1].moves;
        gridSize = levels[currentLevelNumber - 1].gridSize;
        levels[currentLevelNumber - 1] = InitLevel(currentLevelNumber);
        InitGrid();
        transitionTimer = TRANSITION_DURATION;
        inTransition = true;
        if (soundOn) PlaySound(clickSound);
        printf("Transitioning to level %d, Grid %dx%d\n", currentLevelNumber, gridSize, gridSize);
        rowSwapPowerUsed = false;
        specialPowerUsed = false;
    }
}

// Easing Functions
float EaseInOutQuad(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

float EaseOutBounce(float t) {
    if (t < 1 / 2.75) return 7.5625 * t * t;
    if (t < 2 / 2.75) {
        t -= 1.5 / 2.75;
        return 7.5625 * t * t + 0.75;
    }
    if (t < 2.5 / 2.75) {
        t -= 2.25 / 2.75;
        return 7.5625 * t * t + 0.9375;
    }
    t -= 2.625 / 2.75;
    return 7.5625 * t * t + 0.984375;
}