#include <unistd.h> // POSIX sistem çağrıları için (örn. chdir, execve, fork, pipe)
#include <string.h> // string fonksiyonları için (örn. strcmp)
#include <sys/wait.h> // waitpid gibi işlem yönetimi için

// Hata mesajlarını yazan ve 1 döndüren fonksiyon
int err(char *str) {
    while (*str) {
        write(2, str++, 1); // Her karakteri standart hata çıkışına yazar
    }
    write(2, "\n", 1); // Sonuna yeni satır ekler
    return 1; // Hata durumunda 1 döndürür
}

// Komutları çalıştırmak için fonksiyon
int exec(char **av, int i) {
    int fd[2]; // Pipe için dosya tanımlayıcıları
    int status; // Çocuk sürecin durumu için
    int has_pipe = av[i] && !strcmp(av[i], "|"); // Pipe olup olmadığını kontrol eder

    if (!has_pipe) {
        return err("error"); // Pipe yoksa hata döndürür
    }

    if (has_pipe && pipe(fd) == -1) {
        return err("error: fatal\n"); // Pipe oluşturma hatası
    }

    int pid = fork(); // Çocuk süreci oluşturur
    if (!pid) { // Çocuk süreç
        av[i] = 0; // Komut listesini pipe'a ayırır
        // Çıkışları pipe ile eşleştirir
        if (has_pipe && (dup2(fd[1], 1) == -1 || close(fd[0]) == -1 || close(fd[1]) == -1)) {
            return err("error: fatal\n"); // Pipe hatası
        }
        // "cd" komutunu özel olarak işler
        if (!strcmp(*av, "cd")) {
            return cd(av, i); // "cd" komutu için özel işlev çağrısı
        }
        // Komutu çalıştırır
        execve(*av, av, __environ);
        return err("error: cannot execute "), err(*av), err("\n"); // Hata durumunda mesaj döndürür
    }

    // Ana süreç
    waitpid(pid, &status, 0); // Çocuk sürecin bitmesini bekler
    // Girdiyi pipe ile eşleştirir
    if (has_pipe && (dup2(fd[0], 0) == -1 || close(fd[0]) == -1 || close(fd[1]) == -1)) {
        return err("error: fatal\n"); // Pipe hatası
    }
    return WIFEXITED(status) && WEXITSTATUS(status); // Çocuk sürecin durumunu döndürür
}

// Ana fonksiyon
int main(int ac, char **av) {
    int i = 0, status = 0;

    if (ac > 1) { // Komut satırı argümanları varsa
        while (av[i] && av[++i]) { // Argümanları geçer
            av += i; // Argüman listesini ayarlar
            i = 0;
            // Komutları pipe veya noktalı virgül ile ayırır
            while (av[i] && strcmp(av[i], "|") && strcmp(av[i], ";")) {
                i++;
            }

            // "cd" komutu işlenir
            if (!strcmp(av[0], "cd")) {
                if (i != 2) {
                    err("error: cd: bad arguments"); // "cd" için kötü argüman hatası
                } else if (chdir(av[1]) != 0) {
                    err("error: cd: cannot change directory to "), err(av[1]); // "cd" başarısız olduysa hata
                }
            }
            // Diğer komutlar için
            else if (i) {
                status = exec(av, i); // "exec" fonksiyonu ile komutu çalıştırır
            }
        }
    }
    return status; // Programın çıkış durumu
}
