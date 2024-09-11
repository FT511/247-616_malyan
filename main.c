//main:
// 2018-10-11, Yves Roy, creation (247-637 S-0003)

//INCLUSIONS
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h> 
#include "main.h"
#include "piloteSerieUSB.h"
#include "interfaceTouche.h"
#include "interfaceMalyan.h"

//Definitions privees
#define MAIN_LONGUEUR_MAXIMALE 99

//Declarations de fonctions privees:
int main_initialise(void);
void main_termine(void);

//Definitions de variables privees:
//pas de variables privees
int fd; //File Descriptor
int pipeLecture[2];
int pipeEcriture[2];
int pipeErreur[2];

//Definitions de fonctions privees:
int main_initialise(void)
{
  if (piloteSerieUSB_initialise() != 0)
  {
    return -1;
  }
  if (interfaceTouche_initialise() != 0)
  {
    return -1;
  }
  if (interfaceMalyan_initialise() != 0)
  {
    return -1;
  }
  return 0;
}

void main_termine(void)
{
  piloteSerieUSB_termine();
  interfaceTouche_termine();
  interfaceMalyan_termine();
}



void codeDuProcessusParent(void)
{
  int erreur = 0;

  unsigned char toucheLue='D';
  char reponse[MAIN_LONGUEUR_MAXIMALE+1];
  int nombre;

  char read_byte = 0;
  char write_byte = 0;


  close(pipeLecture[1]);   //Fermer extremite ecriture
  close(pipeEcriture[0]);  //Fermer extremite lecture 
  close(pipeErreur[1]);    //Fermer extremite ecriture
 

  fprintf(stdout,"Tapez:\n\r");
  fprintf(stdout, "Q\": pour terminer.\n\r");
  fprintf(stdout, "6\": pour démarrer le ventilateur.\n\r");
  fprintf(stdout, "7\": pour arrêter le ventilateur.\n\r");  
  fprintf(stdout, "8\": donne la position actuelle.\n\r");  
  fprintf(stdout, "P\": va à la position x=20, y=20, z=20.\n\r");  
  fprintf(stdout, "H\": positionne la tête d'impression à l'origine(home).\n\r");  
  fprintf(stdout, "autre chose pour générer une erreur.\n\r");
  fflush(stdout);
  
  while (toucheLue != 'Q')
  {
    printf("Entrez une commande\n");
    toucheLue = interfaceTouche_lit();
    printf("Caractère lu = '%c'\n", toucheLue);
    switch (toucheLue)
    {
      case '6':
        write_byte = '6';
      break;
      case '7':
        write_byte = '7';
      break;
      case '8':
        write_byte = '8';
      break;
      case 'P':
        write_byte = 'P';
      break;
      case 'H':
        write_byte = 'H';
      break;
      default:
      break;
    }

    write(pipeEcriture[1], &write_byte, 1);

    read(pipeErreur[0], &erreur, 1);
    if (erreur == 1)
    {
      printf("erreur lors de la gestion de la commande\n");
      break;
    }
    else
    {
      usleep(100000);                
      nombre = interfaceMalyan_recoitUneReponse(reponse, MAIN_LONGUEUR_MAXIMALE);
      if (nombre < 0)
      {
        erreur = errno;
        printf("main: erreur lors de la lecture: %d\n", erreur);
        perror("erreur: ");
      }
      else
      {
        reponse[nombre] = '\0';
        printf("nombre reçu: %d, réponse: %s", nombre, reponse);      
//      fflush(stdout);
      }
    }

    read(pipeLecture[0], &read_byte, 1);

  }

  write_byte = 'Q';
  write(pipeEcriture[1], &write_byte, 1);
  // Arreter l'enfant 
}


void codeDuProcessusEnfant(void)
{
  int erreur = 0;

  char read_byte = 0;
  //char write_byte = 0;

  close(pipeLecture[0]);   //Fermer extremite lecture
  close(pipeEcriture[1]);  //Fermer extremite ecriture
  close(pipeErreur[0]);    //Fermer extremite lecture

  while(1)
  {
    read(pipeLecture[0], &read_byte, 1);

    switch (read_byte)
    {
      case '6':
        if (interfaceMalyan_demarreLeVentilateur() < 0)
        {
          erreur = 1;
        }
      break;
      case '7':
        if (interfaceMalyan_arreteLeVentilateur() < 0)
        {
          erreur = 1;
        }
      break;
      case '8':
        if (interfaceMalyan_donneLaPosition() < 0)
        {
          erreur = 1;
        }
      break;
      case 'P':
        if (interfaceMalyan_vaALaPosition(20, 20, 20) < 0)
        {
          erreur = 1;
        }
      break;
      case 'H':
        if (interfaceMalyan_retourneALaMaison() < 0)
        {
          erreur = 1;
        }
      break;
      default:
        if (interfaceMalyan_genereUneErreur() < 0)
        {
          erreur = 1;
        }
    }

     write(pipeErreur[1], &erreur, 1);

    if(read_byte == 'Q')
    {
      break;
    }
  }
}

//Definitions de variables publiques:
//pas de variables publiques


//Definitions de fonctions publiques:
int main(int argc,char** argv)
{


  pid_t pid;
  

  

  if (main_initialise())
  {
    printf("main_initialise: erreur\n");
    return 0;
  }

  pid = fork();

  if(pid == 0)
  {
    codeDuProcessusEnfant();
  }

    // Appel fonction Parent
  if(pid != 0)
  {
    codeDuProcessusParent();
    wait(NULL);
  }
  
  
  main_termine();
  return EXIT_SUCCESS;
}
