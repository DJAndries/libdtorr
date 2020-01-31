#include "fs.h"
#include "log.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define ALLOC_BUF_SIZE 64 * 1024

static char* join_path(char* a, char* b) {
  unsigned long alen = strlen(a);
  unsigned long blen = strlen(b);
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

static char* extract_dir(char* path) {
  #ifdef _WIN32
    char sep = '\\';
  #else
    char sep = '/';
  #endif
  char* result = (char*)malloc(sizeof(char) * strlen(path) + 1);
  char* found;
  if (result == 0) {
    return 0;
  }
  strcpy(result, path);
  found = strrchr(result, sep);
  if (found == 0) {
    result[0] = 0;
  } else {
    *found = 0;
  }
  return result;
}

static short unsigned int* create_wide_path(char* path) {
  short unsigned int* wide_path;
  unsigned long wlen = MultiByteToWideChar(CP_UTF8, 0, path, -1, 0, 0);
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

static int allocate_file(char* path, unsigned long length) {
  char zerobuf[ALLOC_BUF_SIZE];
  FILE* f;
  unsigned long count;
  #ifdef _WIN32
    short unsigned int* wide_dir = create_wide_path(path);
    short unsigned int* wide_mode = create_wide_path("wb");
    if (wide_dir == 0 || wide_mode == 0) {
      return 4;
    }
    f = _wfopen(wide_dir, wide_mode);
    free(wide_dir);
    free(wide_mode);
  #else
    f = fopen(path, "wb");
  #endif
  if (f == 0) {
    return 1;
  }
  memset(zerobuf, 0, ALLOC_BUF_SIZE);
  if (setvbuf(f, 0, _IOFBF, ALLOC_BUF_SIZE) != 0) {
    fclose(f);
    return 2;
  }
  while (length != 0) {
    count = ALLOC_BUF_SIZE > length ? length : ALLOC_BUF_SIZE;
    if (fwrite(zerobuf, sizeof(char), count, f) != count) {
      fclose(f);
      return 3;
    }
    length -= count;
  }
  fclose(f);
  return 0;
}

static int init_file(char* download_dir, dtorr_file* meta_file) {
  char* os_file_path;
  char* joined_path;
  char* extracted_dir;
  os_file_path = convert_os_dir(meta_file->cat_path);
  if (os_file_path == 0) {
    return 1;
  }
  joined_path = join_path(download_dir, os_file_path);
  free(os_file_path);
  if (joined_path == 0) {
    return 2;
  }
  extracted_dir = extract_dir(joined_path);
  if (extracted_dir == 0) {
    return 3;
  }
  if (create_dir(extracted_dir) != 0) {
    free(joined_path);
    free(extracted_dir);
    return 4;
  }
  free(extracted_dir);

  if (allocate_file(joined_path, meta_file->length) != 0) {
    free(joined_path);
    return 5;
  }
  free(joined_path);
  return 0;
}

int init_torrent_files(dtorr_config* config, dtorr_torrent* torrent) {
  unsigned long i;
  for (i = 0; i < torrent->file_count; i++) {
    if (init_file(torrent->download_dir, torrent->files[i]) != 0) {
      dlog(config, LOG_LEVEL_ERROR, "File init: Failed to allocate %s", torrent->files[i]->cat_path);
      return 1;
    }
  }
  return 0;
}