# SISOP-4-2026-IT-116
Dian Hanna Simanjuntak (5027251116)
## Reporting
  
## Soal 1 - Save Asisten Kenz  
Pada soal ini dibuat filesystem virtual menggunakan FUSE3. Filesystem akan melakukan passthrough terhadap file asli pada folder `amba_files` dan menambahkan satu file virtual bernama `tujuan.txt`.

File `tujuan.txt` berisi gabungan informasi dari file `1.txt` hingga `7.txt` yang memiliki prefix `KOORD:`.

---  

### Header  

```c
#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>

static const char *source_dir;

static void generate_tujuan(char *result);
```  
Bagian ini digunakan untuk:

- menentukan versi FUSE yang dipakai
- import library yang diperlukan
- membuat variabel global source_dir untuk menyimpan path folder asli
- deklarasi fungsi generate_tujuan()

### Fungsi fullpath()  
```c  
static void fullpath(char fpath[1024], const char *path)
{
    sprintf(fpath, "%s%s", source_dir, path);
}
```  

Fungsi ini digunakan untuk menggabungkan:  
- path folder asli (source_dir)
- path virtual dari FUSE

### Fungsi xmp_getattr()  
```c
static int xmp_getattr(const char *path, struct stat *stbuf,
                       struct fuse_file_info *fi)
{
    (void) fi;

    int res;
    char fpath[1024];

    memset(stbuf, 0, sizeof(struct stat));

    // HANDLE FILE VIRTUAL
    if (strcmp(path, "/tujuan.txt") == 0) {

    char content[4096];

    generate_tujuan(content);

    stbuf->st_mode = S_IFREG | 0444;
    stbuf->st_nlink = 1;
    stbuf->st_size = strlen(content);

    return 0;
    }

    fullpath(fpath, path);

    res = lstat(fpath, stbuf);

    if (res == -1)
        return -errno;

    return 0;
}
```  
Fungsi ini digunakan untuk mengambil atribut file seperti:  
- ukuran file
- permission
- tipe file
  
Jika file yang diakses adalah tujuan.txt, maka atribut dibuat manual karena file tersebut virtual dan tidak benar-benar ada di disk.  
```c
stbuf->st_mode = S_IFREG | 0444;
```  
artinya file hanya bisa dibaca.  

Ukuran file virtual dihitung menggunakan:  
```c
stbuf->st_size = strlen(content);
```  
Untuk file selain `tujuan.txt`, atribut diambil langsung dari filesystem asli menggunakan `lstat()`.  

### Fungsi xmp_readdir()  
```c
static int xmp_readdir(const char *path, void *buf,
                       fuse_fill_dir_t filler,
                       off_t offset,
                       struct fuse_file_info *fi,
                       enum fuse_readdir_flags flags)
{
    (void) offset;
    (void) fi;
    (void) flags;

    DIR *dp;
    struct dirent *de;

    char fpath[1024];

    fullpath(fpath, path);

    dp = opendir(fpath);

    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        filler(buf, de->d_name, NULL, 0, 0);
    }

    closedir(dp);

    // TAMBAH FILE VIRTUAL
    filler(buf, "tujuan.txt", NULL, 0, 0);

    return 0;
}
```  

Fungsi ini digunakan untuk membaca isi direktori.  
  
- membuka folder asli menggunakan opendir()
- membaca seluruh file menggunakan readdir()
- memasukkan nama file ke filesystem virtual menggunakan filler()

Selain file asli, program juga menambahkan file virtual:  
```c
filler(buf, "tujuan.txt", NULL, 0, 0);
```
sehingga file `tujuan.txt` muncul saat menjalankan:  
```c
ls mnt
```

### Fungsi xmp_open()  
```c
static int xmp_open(const char *path, struct fuse_file_info *fi)
{
    int res;
    char fpath[1024];

    // FILE VIRTUAL
    if (strcmp(path, "/tujuan.txt") == 0)
        return 0;

    fullpath(fpath, path);

    res = open(fpath, fi->flags);

    if (res == -1)
        return -errno;

    close(res);

    return 0;
}
```  

Fungsi ini digunakan saat file dibuka.  
Jika file yang dibuka adalah `tujuan.txt`, maka program langsung mengembalikan sukses karena file virtual tidak ada secara fisik.  
Untuk file lain, program membuka file asli menggunakan `open()`.  

### Fungsi generate_tujuan()  
```c
static void generate_tujuan(char *result)
{
    char line[1024];
    char fragment[1024];
    char filepath[1024];

    strcpy(result, "Tujuan Mas Amba: ");

    for (int i = 1; i <= 7; i++) {

        sprintf(filepath, "%s/%d.txt", source_dir, i);

        FILE *fp = fopen(filepath, "r");

        if (!fp)
            continue;

        while (fgets(line, sizeof(line), fp)) {

            if (strncmp(line, "KOORD:", 6) == 0) {

                strcpy(fragment, line + 6);

                fragment[strcspn(fragment, "\n")] = 0;

                strcat(result, fragment);
            }
        }

        fclose(fp);
    }

    strcat(result, "\n");
}
```  

Fungsi ini digunakan untuk membuat isi file virtual `tujuan.txt`.  
  
- membaca file `1.txt` sampai `7.txt`
- mencari baris dengan prefix:  
`KOORD:`  
- mengambil isi setelah prefix tersebut
- menggabungkannya ke variabel result

### Fungsi xmp_read()  
```c  
static int xmp_read(const char *path,
                    char *buf,
                    size_t size,
                    off_t offset,
                    struct fuse_file_info *fi)
{
    (void) fi;

    // HANDLE FILE VIRTUAL
    if (strcmp(path, "/tujuan.txt") == 0) {

        char content[4096];

        generate_tujuan(content);

        size_t len = strlen(content);

        if (offset >= len)
            return 0;

        if (offset + size > len)
            size = len - offset;

        memcpy(buf, content + offset, size);

        return size;
    }

    int fd;
    int res;

    char fpath[1024];

    fullpath(fpath, path);

    fd = open(fpath, O_RDONLY);

    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);

    if (res == -1)
        res = -errno;

    close(fd);

    return res;
}
```  

Fungsi ini digunakan untuk membaca isi file.  
Jika file yang dibaca adalah `tujuan.txt`, maka:  
- isi file dibuat menggunakan `generate_tujuan()`
- data disalin ke buffer menggunakan `memcpy()`  
Untuk file biasa:  
- file dibuka menggunakan `open()`
- isi file dibaca menggunakan  `pread()`

### Struktur Operasi FUSE  
```c  
static const struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open    = xmp_open,
    .read    = xmp_read,
};
```  

Bagian ini digunakan untuk menghubungkan operasi FUSE dengan fungsi yang telah dibuat.  
- operasi read akan memanggil `xmp_read`
- operasi readdir akan memanggil `xmp_readdir`

### Fungsi main()  
```c
int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <source_dir> <mountpoint>\n", argv[0]);
        return 1;
    }

    int foreground = 0;
    int source_index = 1;
    int mount_index = 2;

    // kalau ada -f
    if (strcmp(argv[1], "-f") == 0) {
        foreground = 1;
        source_index = 2;
        mount_index = 3;
    }

    source_dir = realpath(argv[source_index], NULL);

    // argumen untuk fuse
    char *fuse_argv[3];
    int fuse_argc = 0;

    fuse_argv[fuse_argc++] = argv[0];

    if (foreground)
        fuse_argv[fuse_argc++] = "-f";

    fuse_argv[fuse_argc++] = argv[mount_index];

    return fuse_main(fuse_argc, fuse_argv, &xmp_oper, NULL);
}
```  
Fungsi main() digunakan untuk:  
- membaca argument program
- menentukan source directory dan mount point
- menangani mode foreground (-f)
- menjalankan filesystem menggunakan `fuse_main()`

Program dijalankan menggunakan:  
Compile Program  
```bash
gcc kenz_rescue.c `pkg-config fuse3 --cflags --libs` -o kenz_rescue
```  
Menjalankan Program   
```bash
mkdir -p mnt
./kenz_rescue -f amba_files mnt
```  

### Hasil Program  
Isi Folder Mount  
```bash
ls mnt
```

Output:  
```bash
1.txt  2.txt  3.txt  4.txt  5.txt  6.txt  7.txt  tujuan.txt
```  
Isi File Virtual  
```bash  
cat mnt/tujuan.txt
```  
Output:  
```bash  
Tujuan Mas Amba:  -7.957 382728 443728,  112.469 8688227961,  23: 59 WIB
```
