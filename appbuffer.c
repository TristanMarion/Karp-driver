#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#define DEVICE "/dev/testdevice"

int main(void){
	int i, fd;
	char ch, write_buf[100], read_buf[100];

	fd = open(DEVICE, O_RDWR);

	if(fd == -1){
			printf("file %s n'existe pas ou vous n'avez pas les permissions pour faire ca\n", DEVICE);
			exit(-1);
	}
	printf ("r = Affiche le contenu du device\nw = Ecrire du contenu dan le device\nd = Vide le device\nVotre commande : ");
	scanf("%c", &ch);

	switch (ch) {
			case 'w':
					printf("Rentrer votre contenu :");
					scanf(" %[^\n]", write_buf);
					write(fd, write_buf, sizeof(write_buf));
					break;
			case 'r':
					read(fd, read_buf, sizeof(read_buf));
					printf("device: %s\n", read_buf);
					break;
			case 'd':
					write(fd, NULL, sizeof(NULL));
					break;
			default:
					printf("Commande non reconnues\n");
					break;		
	}
	close(fd);
	return 0;
}
