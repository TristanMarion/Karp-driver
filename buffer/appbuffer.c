#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#define DEVICE "/dev/samynaceri"

int main(void)
{
  int i, fd;
  char ch, write_buf[100], read_buf[100];
  
  fd = open(DEVICE, O_RDWR);
  
  if(fd == -1)
  {
    printf("file %s n'existe pas ou vous n'avez pas les permissions pour faire ca\n", DEVICE);
    exit(-1);
  }
  printf("\nr = Afficher le contenu du buffer\n");
  printf("w = Écrire dans le buffer\n");
  printf("d = Vider le buffer\n");
  printf("a = Ajouter du contenu au buffer\n\n");
  printf("Votre commande : ");
  scanf("%c", &ch);
  
  switch (ch)
  {
  case 'w':
    printf("Contenu à écrire : ");
    scanf(" %[^\n]", write_buf);
    write(fd, write_buf, sizeof(write_buf));
    break;
  case 'r':
    read(fd, read_buf, sizeof(read_buf));
    printf("Contenu du buffer : %s\n", read_buf);
    break;
  case 'd':
    write(fd, NULL, sizeof(NULL));
    break;
  case 'a':
    read(fd, read_buf, sizeof(read_buf));
    printf("Contenu à ajouter au buffer : ");
    scanf(" %[^\n]", write_buf);
    strcat(read_buf, write_buf);
    write(fd, read_buf, sizeof(read_buf));
    break;
  default:
    printf("Commande non reconnue\n");
    break;		
  }
  close(fd);
  return 0;
}
