#include <windows.h>
#include <iostream>
#include <random>
#include <string>

using namespace std;

constexpr wchar_t NumeMemorie[] = L"Local\\MemorieComunaLab6";
constexpr wchar_t NumeSemafor[] = L"Local\\SemaforLab6";
constexpr LONG Tinta = 1000;

struct MemoriePartajata {
    LONG numar;
};

void PrinteazaEroare(const char* operatie) {
    cerr << operatie << " a esuat. Cod eroare: " << GetLastError() << '\n';
}

int main(int argc, char* argv[]) {
    string argument;

    // Daca a primit parametru din linia de comanda, il foloseste
    if (argc == 2) {
        argument = argv[1];
    } else {
        // Daca e rulat direct din Code::Blocks, te intreaba interactiv
        cout << "Selecteaza procesul (introdu 1 pentru proces1 sau 2 pentru proces2): ";
        int opt;
        cin >> opt;
        argument = (opt == 1) ? "proces1" : "proces2";
    }

    if (argument != "proces1" && argument != "proces2") {
        cerr << "Optiune invalida!\n";
        return 1;
    }

    string numeProces = (argument == "proces1") ? "Procesul 1" : "Procesul 2";

    // 1. Creare / Deschidere Obiect Mapat in Memorie
    HANDLE hMemorie = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        sizeof(MemoriePartajata),
        NumeMemorie
    );

    if (hMemorie == nullptr) {
        PrinteazaEroare("CreateFileMappingW");
        return 1;
    }

    // 2. Maparea memoriei
    MemoriePartajata* dateComune = (MemoriePartajata*)MapViewOfFile(
        hMemorie,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(MemoriePartajata)
    );

    if (dateComune == nullptr) {
        PrinteazaEroare("MapViewOfFile");
        CloseHandle(hMemorie);
        return 1;
    }

    // 3. Creare / Deschidere Semafor
    HANDLE hSemafor = CreateSemaphoreW(nullptr, 1, 1, NumeSemafor);
    if (hSemafor == nullptr) {
        PrinteazaEroare("CreateSemaphoreW");
        UnmapViewOfFile(dateComune);
        CloseHandle(hMemorie);
        return 1;
    }

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> banul(1, 2);

    cout << numeProces << " a pornit si intra in bucla.\n";

    while (true) {
        // Pas 1: Rezervam semaforul
        DWORD rezultat = WaitForSingleObject(hSemafor, INFINITE);
        if (rezultat != WAIT_OBJECT_0) {
            PrinteazaEroare("WaitForSingleObject");
            break;
        }

        // Citim memoria
        LONG numarCurent = dateComune->numar;

        if (numarCurent >= Tinta) {
            ReleaseSemaphore(hSemafor, 1, nullptr);
            break;
        }

        // Pas 2: Dau cu banul; cat timp cade 2, scriu in memorie urmatorul numar
        while (numarCurent < Tinta) {
            int rezultatBan = banul(gen);

            if (rezultatBan == 2) {
                numarCurent++;
                dateComune->numar = numarCurent;
                cout << numeProces << " a scris: " << numarCurent << '\n';
            } else {
                break;
            }
        }
        // Pas 3
        // Pas 3: Eliberam semaforul
        ReleaseSemaphore(hSemafor, 1, nullptr);

        // Pauza tehnica
        Sleep(5);
    }

    cout << numeProces << " si-a terminat executia.\n";

    CloseHandle(hSemafor);
    UnmapViewOfFile(dateComune);
    CloseHandle(hMemorie);

    return 0;
}
