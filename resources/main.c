#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

//HAMPIR STRESS GARA GARA INI!!!
#if defined(_WIN32)
    #define Rectangle WinRectangle
    #define CloseWindow WinCloseWindow
    #define ShowCursor WinShowCursor
    #define DrawText WinDrawText
    #define DrawTextEx WinDrawTextEx
    #define TextOut WinTextOut
    #define LoadImage WinLoadImage
    #define PlaySound WinPlaySound

    #include <windows.h>
    #include <commdlg.h> 

    #undef Rectangle
    #undef CloseWindow
    #undef ShowCursor
    #undef DrawText
    #undef DrawTextEx
    #undef TextOut
    #undef LoadImage
    #undef PlaySound
#endif

#include "raylib.h"

#define MAX_TRACKS 512       
#define MAX_PATH_LEN 1024    
#define WINDOW_W 1000        
#define WINDOW_H 650         
#define SIDEBAR_W 260        


static char playlist[MAX_TRACKS][MAX_PATH_LEN];
static int track_count = 0;   
static int current_track = 0; 

static float musicVolume = 0.8f; 

// FUNGSI: IMPORT FILE
static char* OpenFilePicker(void)
{
    static char buffer[MAX_PATH_LEN];
    memset(buffer, 0, sizeof(buffer));

#if defined(_WIN32)
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn)); 
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = sizeof(buffer);
    ofn.lpstrFilter = "File MP3 (*.mp3)\0*.mp3\0Semua File (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileName(&ofn))
    {
        return buffer;
    }
#endif
    return NULL;
}

// FUNGSI: MEMUAT DAN MEMUTAR LAGU
static void LoadAndPlayTrack(int idx, Music *music, bool *musicLoaded, bool *isPlaying)
{
    if (idx < 0 || idx >= track_count) return;

    if (*musicLoaded)
    {
        StopMusicStream(*music);
        UnloadMusicStream(*music);
        *musicLoaded = false;
        *isPlaying = false;
    }

    *music = LoadMusicStream(playlist[idx]); 
    
    PlayMusicStream(*music);
    
    if (!IsMusicStreamPlaying(*music))
    {
        TraceLog(LOG_WARNING, "Gagal load lagu: %s", playlist[idx]);
        return;
    }

    SetMusicVolume(*music, musicVolume);
    *musicLoaded = true;
    *isPlaying = true;

    TraceLog(LOG_INFO, "Sedang memutar: %s", playlist[idx]);
}

//FUNGSI: TOMBOL GUI SEDERHANA
static bool GuiButton(Rectangle rec, const char *text)
{
    Vector2 mp = GetMousePosition();
    bool hover = CheckCollisionPointRec(mp, rec);
    bool clicked = hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    Color bg = hover ? (Color){60,60,60,255} : (Color){45,45,45,255};
    
    DrawRectangleRec(rec, bg);
    DrawText(text, (int)(rec.x + 12), (int)(rec.y + rec.height/2 - 10), 20, RAYWHITE);
    
    return clicked;
}

//FUNGSI: SLIDER HORIZONTAL
static float GuiHSlider(Rectangle rec, float value)
{
    DrawRectangleRec(rec, (Color){60,60,60,200});
    
    Rectangle filled = rec;
    filled.width = rec.width * value;
    DrawRectangleRec(filled, (Color){30,215,96,220});

    float handleX = rec.x + rec.width * value;
    DrawCircle((int)handleX, (int)(rec.y + rec.height/2), rec.height*0.6f, (Color){200,200,200,255});

    Vector2 mp = GetMousePosition();
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
    {
        if (CheckCollisionPointRec(mp, rec))
        {
            float newVal = (mp.x - rec.x) / rec.width;
            if (newVal < 0) newVal = 0;
            if (newVal > 1) newVal = 1;
            return newVal;
        }
    }
    return value;
}

//FUNGSI:MENDETEKSI LAGU DI FOLDER MUSIC
static void AutoScanMusic()
{
    if (!DirectoryExists("music"))
    {
        TraceLog(LOG_WARNING, "Folder 'music' tidak ditemukan.");
    }

    FilePathList files = LoadDirectoryFiles("music/");

    for (int i = 0; i < files.count && track_count < MAX_TRACKS; i++)
    {
        const char *p = files.paths[i];

        const char *ext = strrchr(p, '.');
        if (ext && (strcmp(ext, ".mp3") == 0 || strcmp(ext, ".MP3") == 0))
        {
            strncpy(playlist[track_count], p, MAX_PATH_LEN - 1);
            playlist[track_count][MAX_PATH_LEN - 1] = '\0';
            track_count++;
        }
    }
    UnloadDirectoryFiles(files);
}

// PROGRAM UTAMA
int main(void)
{
    AutoScanMusic();

    InitWindow(WINDOW_W, WINDOW_H, "TUGAS UAS AUDIO PLAYER - Raylib");
    InitAudioDevice(); 

    if (track_count == 0)
    {
        TraceLog(LOG_WARNING, "Folder 'music/' kosong atau belum ada.");
    }

    Music music = { 0 };
    bool musicLoaded = false;
    bool isPlaying = false;

    if (track_count > 0) 
    {
        LoadAndPlayTrack(0, &music, &musicLoaded, &isPlaying);
    }

    SetTargetFPS(60); 

    int scrollOffset = 0;
    const int rowHeight = 40;
    const int visibleRows = (WINDOW_H - 120) / rowHeight;

    while (!WindowShouldClose()) 
    {
        // A. UPDATE STATUS MUSIK
        if (musicLoaded)
        {
            UpdateMusicStream(music); 
            
            if (!IsMusicStreamPlaying(music) && isPlaying)
            {
                int next = (current_track + 1) % (track_count > 0 ? track_count : 1);
                current_track = next;
                LoadAndPlayTrack(current_track, &music, &musicLoaded, &isPlaying);
            }
        }

        // B. SHORTCUT KEYBOARD
        if (IsKeyPressed(KEY_SPACE))
        {
            if (musicLoaded)
            {
                if (isPlaying) PauseMusicStream(music);
                else ResumeMusicStream(music);
                isPlaying = !isPlaying;
            }
        }
        if (IsKeyPressed(KEY_N))
        {
            if (track_count > 0)
            {
                current_track = (current_track + 1) % track_count;
                LoadAndPlayTrack(current_track, &music, &musicLoaded, &isPlaying);
            }
        }
        if (IsKeyPressed(KEY_P))
        {
            if (track_count > 0)
            {
                current_track--;
                if (current_track < 0) current_track = track_count - 1; 
                LoadAndPlayTrack(current_track, &music, &musicLoaded, &isPlaying);
            }
        }
        if (IsKeyPressed(KEY_RIGHT) && musicLoaded)
        {
            SeekMusicStream(music, GetMusicTimePlayed(music) + 5.0f);
        }
        if (IsKeyPressed(KEY_LEFT) && musicLoaded)
        {
            SeekMusicStream(music, GetMusicTimePlayed(music) - 5.0f);
        }

        // C. SCROLL MOUSE
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f && track_count > visibleRows)
        {
            scrollOffset -= (int)wheel;
            if (scrollOffset < 0) scrollOffset = 0;
            if (scrollOffset > track_count - visibleRows) scrollOffset = track_count - visibleRows;
        }

        // D. GAMBAR (DRAWING)
        BeginDrawing();
        ClearBackground((Color){18,18,18,255}); 

        //1. SIDEBAR KIRI
        DrawRectangle(0, 0, SIDEBAR_W, WINDOW_H, (Color){12,12,12,255});
        DrawText("MY MUSIC", 30, 18, 22, RAYWHITE);
        DrawText("Daftar Lagu", 20, 60, 16, (Color){180,180,180,255});

        Rectangle listArea = (Rectangle){ 10, 90, SIDEBAR_W - 20, WINDOW_H - 150 };
        DrawRectangleRec(listArea, (Color){20,20,20,255});
        DrawRectangleLinesEx(listArea, 2, (Color){40,40,40,255});

        int startIndex = scrollOffset;
        for (int i = 0; i < visibleRows; i++)
        {
            int idx = startIndex + i;
            if (idx >= track_count) break; 
            
            float y = listArea.y + i * rowHeight;
            Rectangle itemRec = (Rectangle){ listArea.x, y, listArea.width, rowHeight - 6 };

            if (idx == current_track) DrawRectangleRec(itemRec, (Color){30,120,70,180}); 
            else DrawRectangleRec(itemRec, (Color){28,28,28,220}); 

            const char *fileNameDisplay = GetFileName(playlist[idx]);

            char textBuffer[256];
            snprintf(textBuffer, 256, "%d. %s", idx+1, fileNameDisplay);

            // MEMBATASI PANJANG TEKS AGAR TIDAK MELAMPAUI KOTAK
            int maxTextWidth = (int)itemRec.width - 20; 
            
            if (MeasureText(textBuffer, 14) > maxTextWidth)
            {
                while (MeasureText(textBuffer, 14) > maxTextWidth - 15)
                {
                    int len = strlen(textBuffer);
                    if (len > 0) textBuffer[len - 1] = '\0'; 
                    else break;
                }
                strcat(textBuffer, "..."); 
            }

            DrawText(textBuffer, (int)(itemRec.x + 6), (int)(itemRec.y + 8), 14, RAYWHITE);

            Vector2 mp = GetMousePosition();
            if (CheckCollisionPointRec(mp, itemRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                current_track = idx;
                LoadAndPlayTrack(current_track, &music, &musicLoaded, &isPlaying);
            }
        }

        // 2. AREA UTAMA 
        DrawRectangle(SIDEBAR_W + 20, 18, WINDOW_W - SIDEBAR_W - 40, 120, (Color){28,28,28,255});
        DrawText("Sedang Diputar", SIDEBAR_W + 40, 28, 20, (Color){200,200,200,255});

       if (track_count > 0)
        {
            // --- LOGIKA POTONG TEKS (JUDUL UTAMA) ---
            const char *titleRaw = GetFileName(playlist[current_track]);
            char titleBuffer[256];
            snprintf(titleBuffer, 256, "%s", titleRaw); // Salin ke buffer

            // Hitung batas lebar. Lebar kotak dikurang sedikit biar ada sisa kanan-kiri.
            // Rumus: (Lebar Window - Lebar Sidebar - 100 pixel padding)
            int maxTitleWidth = WINDOW_W - SIDEBAR_W - 100; 
            int fontSize = 16; // Ukuran font judul

            // Cek kalau kepanjangan
            if (MeasureText(titleBuffer, fontSize) > maxTitleWidth)
            {
                // Potong huruf dari belakang sampai muat
                while (MeasureText(titleBuffer, fontSize) > maxTitleWidth - 20)
                {
                    int len = strlen(titleBuffer);
                    if (len > 0) titleBuffer[len - 1] = '\0'; 
                    else break;
                }
                strcat(titleBuffer, "..."); // Tambah titik-titik
            }

            // Gambar teks hasil sunat
            DrawText(titleBuffer, SIDEBAR_W + 40, 58, fontSize, (Color){180,220,180,255});
            // ----------------------------------------
        }
        else
        {
            DrawText("Belum ada lagu... Upload dulu!", SIDEBAR_W + 40, 58, 16, (Color){200,120,120,255});
        }

        Rectangle artRec = (Rectangle){ SIDEBAR_W + 40, 150, 220, 220 };
        DrawRectangleRec(artRec, (Color){40,40,40,255});
        DrawRectangleLinesEx(artRec, 2, (Color){70,70,70,255});
        DrawText("Cooming soon", (int)(artRec.x+40), (int)(artRec.y + artRec.height/2 - 8), 18, (Color){150,150,150,255});

        // 3. TOMBOL KONTROL 
        float controlsX = SIDEBAR_W + 300;
        float controlsY = 220;

        float uploadY = 600;
        float deleteY = 600;
        
        Rectangle prevBtn = (Rectangle){ controlsX, controlsY, 90, 40 };
        Rectangle playBtn = (Rectangle){ controlsX + 110, controlsY, 100, 40 };
        Rectangle nextBtn = (Rectangle){ controlsX + 230, controlsY, 90, 40 };
        Rectangle uploadBtn = (Rectangle){ controlsX + -410, uploadY, 100, 40 };
        Rectangle deleteBtn = (Rectangle){ controlsX + -550, deleteY, 100, 40 };

        if (GuiButton(prevBtn, "<< Prev"))
        {
            if (track_count > 0)
            {
                current_track--;
                if (current_track < 0) current_track = track_count - 1;
                LoadAndPlayTrack(current_track, &music, &musicLoaded, &isPlaying);
            }
        }

        if (GuiButton(playBtn, isPlaying ? "Pause" : "Play"))
        {
            if (musicLoaded)
            {
                if (isPlaying) PauseMusicStream(music);
                else ResumeMusicStream(music);
                isPlaying = !isPlaying;
            }
            else if (track_count > 0)
            {
                LoadAndPlayTrack(current_track, &music, &musicLoaded, &isPlaying);
            }
        }

        if (GuiButton(nextBtn, "Next >>"))
        {
            if (track_count > 0)
            {
                current_track = (current_track + 1) % track_count;
                LoadAndPlayTrack(current_track, &music, &musicLoaded, &isPlaying);
            }
        }


        // --- TOMBOL UPLOAD ---
        if (GuiButton(uploadBtn, "Upload"))
        {
            char *sourcePath = OpenFilePicker(); 
            
            if (sourcePath && track_count < MAX_TRACKS)
            {
                const char *fileName = GetFileName(sourcePath);
                
                char destPath[MAX_PATH_LEN];
                snprintf(destPath, MAX_PATH_LEN, "music/%s", fileName);
                
                
                int dataSize = 0; 
                unsigned char *fileData = LoadFileData(sourcePath, &dataSize);
                
                if (fileData)
                {
                    if (SaveFileData(destPath, fileData, dataSize))
                    {
                        TraceLog(LOG_INFO, "Upload Sukses: %s", fileName);

                        strncpy(playlist[track_count], destPath, MAX_PATH_LEN - 1);
                        playlist[track_count][MAX_PATH_LEN - 1] = '\0';
                        track_count++;

                        current_track = track_count - 1;
                        LoadAndPlayTrack(current_track, &music, &musicLoaded, &isPlaying);
                    }
                    else
                    {
                        TraceLog(LOG_WARNING, "Gagal simpan file ke folder music/.");
                    }
                    UnloadFileData(fileData); 
                }
            }
        }
        if (GuiButton(deleteBtn, "Hapus"))
        {
            if (track_count > 0)
            {
                //1. menghentikan musik jika sedang diputar
                if (musicLoaded)
                {
                    StopMusicStream(music);
                    UnloadMusicStream(music);
                    musicLoaded = false;
                    isPlaying = false;
                }

                // 2. Hapus File Fisik dari Folder
                const char *fileToDelete = playlist[current_track];
                
                if (remove(fileToDelete) == 0) 
                {
                    TraceLog(LOG_INFO, "Berhasil menghapus: %s", fileToDelete);

                    // 3. Hapus dari Array Playlist (Geser data ke kiri) 
                    
                    for (int i = current_track; i < track_count - 1; i++)
                    {
                        strncpy(playlist[i], playlist[i + 1], MAX_PATH_LEN);
                    }

                    track_count--; // mengurangi jumlah total lagu

                    // 4. Putar Lagu Pengganti 
                    if (track_count > 0)
                    {
                        
                        if (current_track >= track_count) current_track = 0;
                        
                        
                        LoadAndPlayTrack(current_track, &music, &musicLoaded, &isPlaying);
                    }
                    else
                    {
                        current_track = 0; 
                    }
                }
                else
                {
                    TraceLog(LOG_WARNING, "Gagal, coba lagi");
                
                    LoadAndPlayTrack(current_track, &music, &musicLoaded, &isPlaying);
                }
            }
        }

        // 4. PROGRESS BAR 
        Rectangle progressBar = (Rectangle){ SIDEBAR_W + 40, 400, WINDOW_W - SIDEBAR_W - 120, 18 };
        DrawRectangleRec(progressBar, (Color){50,50,50,220}); 
        
        if (musicLoaded)
        {
            float played = GetMusicTimePlayed(music);
            float length = GetMusicTimeLength(music);
            float progress = (length > 0.0001f) ? (played / length) : 0.0f;
            
            Rectangle filled = progressBar;
            filled.width = progressBar.width * progress;
            DrawRectangleRec(filled, (Color){30,215,96,220});

            DrawText(TextFormat("%02d:%02d", (int)(played/60.0f), (int)fmod(played,60.0f)),
                     (int)progressBar.x, (int)(progressBar.y - 20), 14, RAYWHITE);
            DrawText(TextFormat("%02d:%02d", (int)(length/60.0f), (int)fmod(length,60.0f)),
                     (int)(progressBar.x + progressBar.width - 60), (int)(progressBar.y - 20), 14, RAYWHITE);

            Vector2 mp = GetMousePosition();
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mp, progressBar))
            {
                float newProgress = (mp.x - progressBar.x) / progressBar.width;
                if (newProgress < 0) newProgress = 0;
                if (newProgress > 1) newProgress = 1;
                
                SeekMusicStream(music, length * newProgress);
            }
        }
        else
        {
            DrawText("Tidak ada musik.", (int)(progressBar.x), (int)(progressBar.y - 20), 14, (Color){200,100,100,255});
        }

        //5. VOLUME CONTROL
        Rectangle volRec = (Rectangle){ SIDEBAR_W + 40, 450, 300, 16 };
        DrawText("Volume", SIDEBAR_W + 40, 430, 14, RAYWHITE);
        musicVolume = GuiHSlider(volRec, musicVolume);
        if (musicLoaded) SetMusicVolume(music, musicVolume);

        DrawText("Spasi: Play/Pause  |  N: Next  |  P: Prev  |  Panah: Mundur/Maju 5s",
                 SIDEBAR_W + 40, WINDOW_H - 30, 12, (Color){180,180,180,255});

        EndDrawing();
    }
    
    if (musicLoaded)
    {
        StopMusicStream(music);
        UnloadMusicStream(music);
    }
    CloseAudioDevice();
    CloseWindow(); 

    return 0;
}