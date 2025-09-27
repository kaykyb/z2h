#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "parse.h"

int update_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *updatestring)
{
  char *name = strtok(updatestring, ",");
  char *hours = strtok(NULL, ",");
  int parsed_hours = atoi(hours);

  for (int i = 0; i < dbhdr->count; i++)
  {
    if (strcmp(employees[i].name, name))
      continue;

    employees[i].hours = parsed_hours;
  }

  return STATUS_SUCCESS;
}

int remove_employee(struct dbheader_t *dbhdr, struct employee_t *employees, struct employee_t **employeesOut, char *removestring)
{
  int kept = 0;

  struct employee_t *out = calloc(dbhdr->count, sizeof(struct employee_t));
  if (!out)
  {
    printf("Malloc failed\n");
    return STATUS_ERROR;
  }

  for (size_t i = 0; i < dbhdr->count; i++)
  {
    if (strcmp(employees[i].name, removestring))
    {
      out[kept] = employees[i];
      kept++;
    }
  }

  out = realloc(out, kept * sizeof(struct employee_t));
  if (!out)
  {
    printf("Malloc failed\n");
    return STATUS_ERROR;
  }

  dbhdr->count = kept;
  *employeesOut = out;

  return STATUS_SUCCESS;
}

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees)
{
  for (int i = 0; i < dbhdr->count; i++)
  {
    printf("Employee %d\n", i);
    printf("\tName:    %s\n", employees[i].name);
    printf("\tAddress: %s\n", employees[i].address);
    printf("\tHours:   %d hours\n", employees[i].hours);
  }
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t **employeesptr, char *addstring)
{
  if (dbhdr == NULL || employeesptr == NULL || addstring == NULL)
  {
    return STATUS_ERROR;
  }

  dbhdr->count++;

  struct employee_t *employees = realloc(*employeesptr, dbhdr->count * sizeof(struct employee_t));
  *employeesptr = employees;

  char *name = strtok(addstring, ",");
  char *addr = strtok(NULL, ",");
  char *hours = strtok(NULL, ",");

  strncpy(employees[dbhdr->count - 1].name, name, sizeof(employees[dbhdr->count - 1].name));
  strncpy(employees[dbhdr->count - 1].address, addr, sizeof(employees[dbhdr->count - 1].address));

  employees[dbhdr->count - 1].hours = atoi(hours);

  return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut)
{
  if (fd < 0)
  {
    printf("Got a bad FD from the user\n");
    return STATUS_ERROR;
  }

  int count = dbhdr->count;

  struct employee_t *employees = calloc(count, sizeof(struct employee_t));
  if (employees == NULL)
  {
    printf("Malloc failed\n");
    return STATUS_ERROR;
  }

  read(fd, employees, count * sizeof(struct employee_t));

  for (int i = 0; i < count; i++)
  {
    employees[i].hours = ntohl(employees[i].hours);
  }

  *employeesOut = employees;
  return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees)
{
  if (fd < 0)
  {
    printf("Got a bad FD from the user\n");
    return STATUS_ERROR;
  }

  int dbhdr_count = dbhdr->count;
  int new_filesize = sizeof(struct dbheader_t) + (sizeof(struct employee_t) * dbhdr_count);

  dbhdr->magic = htonl(dbhdr->magic);
  dbhdr->filesize = htonl(new_filesize);
  dbhdr->count = htons(dbhdr->count);
  dbhdr->version = htons(dbhdr->version);

  lseek(fd, 0, SEEK_SET);
  write(fd, dbhdr, sizeof(struct dbheader_t));
  if (ftruncate(fd, new_filesize) == -1)
  {
    printf("Failed to truncate file\n");
    return STATUS_ERROR;
  };

  for (int i = 0; i < dbhdr_count; i++)
  {
    employees[i].hours = htonl(employees[i].hours);
    write(fd, &employees[i], sizeof(struct employee_t));
  }

  return STATUS_SUCCESS;
}

int validate_db_header(int fd, struct dbheader_t **headerOut)
{
  if (fd < 0)
  {
    printf("Got a bad FD from the user\n");
    return STATUS_ERROR;
  }

  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
  if (header == NULL)
  {
    printf("Malloc failed to create a db header\n");
    return STATUS_ERROR;
  }

  if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t))
  {
    perror("read");
    free(header);
    return STATUS_ERROR;
  }

  header->version = ntohs(header->version);
  header->count = ntohs(header->count);
  header->magic = ntohl(header->magic);
  header->filesize = ntohl(header->filesize);

  if (header->magic != HEADER_MAGIC)
  {
    printf("Improper header magic\n");
    free(header);
    return -1;
  }

  if (header->version != 1)
  {
    printf("Improper header version\n");
    free(header);
    return -1;
  }

  struct stat dbstat = {0};
  fstat(fd, &dbstat);
  if (header->filesize != dbstat.st_size)
  {
    printf("Corrupted database\n");
    free(header);
    return -1;
  }

  *headerOut = header;
  return 0;
}

int create_db_header(struct dbheader_t **headerOut)
{
  if (headerOut == NULL)
  {
    return STATUS_ERROR;
  }

  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
  if (header == NULL)
  {
    return STATUS_ERROR;
  }

  header->version = 0x1;
  header->count = 0;
  header->magic = HEADER_MAGIC;
  header->filesize = sizeof(struct dbheader_t);

  *headerOut = header;

  return STATUS_SUCCESS;
}
