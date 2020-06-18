#include "dtorr/fs.h"
#include "log.h"
#include "util.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define lfseek _fseeki64
#else
#define _FILE_OFFSET_BITS 64
#define lfseek fseeko
#endif

#define ALLOC_BUF_SIZE 16 * 1024 * 1024

static char* join_path(char* a, char* b) {
  unsigned long long alen = strlen(a);
  unsigned long long blen = strlen(b);
  char* result = (char*)malloc(sizeof(char) * (alen + blen + 2));
  if (result == 0) {
    return 0;
  }
  memcpy(result, a, alen);
  #ifdef _WIN32
    result[alen] = '\\';
  #else
    result[alen] = '/';
  #endif
  memcpy(result + alen + 1, b, blen);
  result[alen + 1 + blen] = 0;
  return result;
}

static char* convert_os_dir(char* dir) {
  char* result = (char*)malloc(sizeof(char) * strlen(dir) + 1);
  if (result == 0) {
    return 0;
  }
  char* current_char;
  strcpy(result, dir);
  #ifdef _WIN32
    current_char = strchr(result, '/');
    while (current_char != 0) {
      *current_char = '\\';
      current_char = strchr(current_char, '/');
    }
  #endif
  return result;
}

static short unsigned int* create_wide_path(char* path) {
  short unsigned int* wide_path;
  unsigned long long wlen = MultiByteToWideChar(CP_UTF8, 0, path, -1, 0, 0);
  if (wlen == 0) {
    return 0;
  }
  wide_path = (short unsigned int*)malloc(wlen * sizeof(short unsigned int));
  if (wide_path == 0) {
    return 0;
  }

  if (MultiByteToWideChar(CP_UTF8, 0, path, -1, wide_path, wlen) == 0) {
    free(wide_path);
    return 0;
  }
  return wide_path;
}

static int create_dir(char* path) {
  #ifdef _WIN32
    int result;

    short unsigned int* wide_dir = create_wide_path(path);
    if (wide_dir == 0) {
      return 1;
    }

    result = CreateDirectoryW(wide_dir, 0);

    free(wide_dir);

    if (result == 0 && GetLastError() == ERROR_ALREADY_EXISTS) {
      return 0;
    }
    return result == 0 ? 3 : 0;
  #else
  #endif
}

static FILE* open_file(char* path, char* mode) {
  FILE* f;
  #ifdef _WIN32
    short unsigned int* wide_dir = create_wide_path(path);
    short unsigned int* wide_mode = create_wide_path(mode);
    if (wide_dir == 0 || wide_mode == 0) {
      return 0;
    }
    f = _wfopen(wide_dir, wide_mode);
    free(wide_dir);
    free(wide_mode);
  #else
    f = fopen(path, mode);
  #endif
  return f;
}

static char* file_full_path(char* download_dir, dtorr_file* meta_file) {
  char* os_file_path;
  char* joined_path;
  os_file_path = convert_os_dir(meta_file->cat_path);
  if (os_file_path == 0) {
    return 0;
  }
  joined_path = join_path(download_dir, os_file_path);
  free(os_file_path);
  return joined_path;
}

static int allocate_file(char* path, unsigned long long length) {
  char* zerobuf = (char*)malloc(sizeof(char) * ALLOC_BUF_SIZE);
  FILE* f;
  unsigned int count;

  if (zerobuf == 0) {
    return 1;
  }

  memset(zerobuf, 0, ALLOC_BUF_SIZE);

  f = open_file(path, "wb");
  if (f == 0) {
    free(zerobuf);
    return 1;
  }
  setvbuf(f, 0,_IONBF, 0);
  while (length != 0) {
    count = ALLOC_BUF_SIZE > length ? length : ALLOC_BUF_SIZE;
    if (fwrite(zerobuf, sizeof(char), count, f) != count) {
      fclose(f);
      free(zerobuf);
      return 3;
    }
    length -= count;
  }
  free(zerobuf);
  fclose(f);
  return 0;
}

static int init_file(char* download_dir, dtorr_file* meta_file) {
  char* joined_path = file_full_path(download_dir, meta_file);
  unsigned long long i;
  char *dir = 0;
  char *dir_part;

  if (joined_path == 0) {
    return 1;
  }

  if (meta_file->path->type == DTORR_LIST) {
    /* create nested dirs */
    for (i = 0; i < meta_file->path->len - 1; i++) {
      dir_part = ((dtorr_node**)meta_file->path->value)[i]->value;
      dir = join_path(dir != 0 ? dir : download_dir, dir_part);

      if (create_dir(dir) != 0) {
        free(dir);
        free(joined_path);
        return 4;
      }
    }
  }

  if (dir != 0) {
    free(dir);
  }

  if (allocate_file(joined_path, meta_file->length) != 0) {
    free(joined_path);
    return 5;
  }
  free(joined_path);
  return 0;
}

int init_torrent_files(dtorr_config* config, dtorr_torrent* torrent) {
  unsigned long long i;
  for (i = 0; i < torrent->file_count; i++) {
    if (init_file(torrent->download_dir, torrent->files[i]) != 0) {
      dlog(config, LOG_LEVEL_ERROR, "File init: Failed to allocate %s", torrent->files[i]->cat_path);
      return 1;
    }
  }
  return 0;
}

static long long rw_for_file(dtorr_config* config, char* download_dir, dtorr_file* meta_file, unsigned long long file_offset, char* buf, unsigned long long buf_size, char is_write) {
  char* full_path;
  FILE* f;
  long long result;
  long long len;
  if ((full_path = file_full_path(download_dir, meta_file)) == 0) {
    return -1;
  }
  if ((f = open_file(full_path, "rb+")) == 0) {
    dlog(config, LOG_LEVEL_DEBUG, "Failed to open file");
    free(full_path);
    return -2;
  }
  free(full_path);
  if (lfseek(f, file_offset, SEEK_SET) != 0) {
    dlog(config, LOG_LEVEL_DEBUG, "Failed to seek file");
    fclose(f);
    return -3;
  }
  len = (meta_file->length - file_offset) >= buf_size ? buf_size : (meta_file->length - file_offset);
  if (is_write == 1) {
    dlog(config, LOG_LEVEL_DEBUG, "Writing " SPEC_LLD " bytes at offset " SPEC_LLU " in file %s", len, file_offset, meta_file->cat_path);
    if ((result = fwrite(buf, sizeof(char), len, f)) != len) {
      fclose(f);
      return -4;
    }
  } else {
    dlog(config, LOG_LEVEL_DEBUG, "Reading " SPEC_LLD " bytes at offset " SPEC_LLU " in file %s", len, file_offset, meta_file->cat_path);
    if ((result = fread(buf, sizeof(char), len, f)) != len) {
      fclose(f);
      return -4;
    }
  }
  fclose(f);
  return result;
}

int rw_piece(dtorr_config* config, dtorr_torrent* torrent, unsigned long long index, char* buf, unsigned long long buf_size, char is_write) {
  unsigned long long piece_global_offset = torrent->piece_length * index;
  unsigned long long i;
  unsigned long long file_global_offset = 0;
  unsigned long long file_offset;
  long long io_result;
  dtorr_file* meta_file;
  for (i = 0; i < torrent->file_count && buf_size != 0; i++) {

    meta_file = torrent->files[i];

    if ((file_global_offset + meta_file->length) > piece_global_offset) {

      file_offset = piece_global_offset - file_global_offset;
      io_result = rw_for_file(config, torrent->download_dir, meta_file, file_offset, buf, buf_size, is_write);

      if (io_result < 0) {

        dlog(config, LOG_LEVEL_ERROR, "Failed i/o for file %s", torrent->files[i]->cat_path);
        return 1;
      }

      buf += io_result;
      buf_size -= io_result;
      piece_global_offset += io_result;
    }
    file_global_offset += meta_file->length;
  } 
  return 0;
}
