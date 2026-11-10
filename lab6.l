#include <iostream>
#include <string>
#include <random>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>

using namespace std;

constexpr char NumeMemorie[] = "/memorie_comuna_lab6";
constexpr char NumeSemafor[] = "/semafor_lab6";
constexpr long Tinta = 1000;

struct MemoriePartajata {
    long numar;
};

void PrinteazaEroare(const char* operatie) {
    perror(operatie);
}

int main(int argc, char* argv[]) {
    string argument;

    if (argc == 2) {
        argument = argv[1];
    } else {
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

    int shm_fd = shm_open(NumeMemorie, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        PrinteazaEroare("shm_open a esuat");
        return 1;
    }

    if (ftruncate(shm_fd, sizeof(MemoriePartajata)) == -1) {
        PrinteazaEroare("ftruncate a esuat");
        close(shm_fd);
        return 1;
    }

    MemoriePartajata* dateComune = (MemoriePartajata*)mmap(
        NULL,
        sizeof(MemoriePartajata),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        shm_fd,
        0
    );

    if (dateComune == MAP_FAILED) {
        PrinteazaEroare("mmap a esuat");
        close(shm_fd);
        return 1;
    }

    sem_t* hSemafor = sem_open(NumeSemafor, O_CREAT, 0666, 1);
    if (hSemafor == SEM_FAILED) {
        PrinteazaEroare("sem_open a esuat");
        munmap(dateComune, sizeof(MemoriePartajata));
        close(shm_fd);
        return 1;
    }

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> banul(1, 2);

    cout << numeProces << " a pornit si intra in bucla.\n";

    while (true) {
        if (sem_wait(hSemafor) == -1) {
            PrinteazaEroare("sem_wait a esuat");
            break;
        }

        long numarCurent = dateComune->numar;

        if (numarCurent >= Tinta) {
            sem_post(hSemafor);
            break;
        }

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

        sem_post(hSemafor);

        usleep(5000);
    }

    cout << numeProces << " si-a terminat executia.\n";

    sem_close(hSemafor);
    munmap(dateComune, sizeof(MemoriePartajata));
    close(shm_fd);

    if (argument == "proces1") {
        shm_unlink(NumeMemorie);
        sem_unlink(NumeSemafor);
    }

    return 0;
}
