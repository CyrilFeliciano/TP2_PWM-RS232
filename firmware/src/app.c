/*******************************************************************************
  MPLAB Harmony Application Source File
  
  Company:
    Microchip Technology Inc.
  
  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It 
    implements the logic of the application's state machine and it may call 
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2013-2014 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/
// DOM-IGNORE-END


// *****************************************************************************
// *****************************************************************************
// Section: Included Files 
// *****************************************************************************
// *****************************************************************************

#include "app.h"
#include "Mc32gest_RS232.h"
#include <stdint.h>

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.
    
    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;
S_pwmSettings pData;
S_pwmSettings PWMData;
S_pwmSettings PWMDataToSend;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Fonction :
    void callback_timer1(void)

  R�sum� :
    Fonction de rappel du timer 1.

  Description :
    Cette fonction est appel�e � chaque d�clenchement du timer 1, qui est
    configur� pour des d�clenchements tous les 100 ms. Elle maintient un
    compteur statique, et lorsqu'il atteint la valeur 150 (3 secondes),
    elle d�clenche un changement d'�tat de l'application vers APP_STATE_SERVICE_TASKS
    en appelant la fonction APP_UpdateState. Elle est r�appell�e toutes les 100ms 

  Remarques :
    - La fonction est associ�e � un timer configur� pour des
      d�clenchements p�riodiques tous les 100 ms.

*/
// *****************************************************************************
void callback_timer1(void)
{
    static uint8_t compteur3s = 0;
        // D�clencher un changement d'�tat vers APP_STATE_SERVICE_TASKS
        if (compteur3s <= 149)
        {
            APP_UpdateState(APP_STATE_INIT);
            compteur3s++;
        }
        else
        {
            APP_UpdateState(APP_STATE_SERVICE_TASKS);
        }
}

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
/* Fonction :
    void APP_UpdateState(APP_STATES NewState)

  R�sum� :
    Met � jour l'�tat global de l'application.

  Description :
    Cette fonction met � jour l'�tat global du switch avec la nouvelle
    valeur sp�cifi�e en tant que param�tre. Elle doit �tre appel�e pour changer
    l'�tat de l'application selon les besoins de la logique de la machine � �tats.

  Remarques :
    La variable globale appData.state est mise � jour directement avec la nouvelle
    valeur sp�cifi�e en param�tre. Aucune valeur de retour n'est fournie, car la
    modification est effectu�e directement sur la variable globale d'�tat.

    Cette fonction est utilis�e pour faciliter la gestion de l'�tat de l'application
    dans la machine � �tats principale de l'application.
*/
// *****************************************************************************
void APP_UpdateState(APP_STATES NewState)
{
    // Met � jour l'�tat de l'application avec la nouvelle valeur
    appData.state = NewState;
    
    // Aucune sortie explicite, car la mise � jour est effectu�e directement sur la variable d'�tat globale.
    // La fonction n'a pas de valeur de retour (void).
}


// *****************************************************************************

// *****************************************************************************
/* Fonction :
    void Allume_Leds(uint8_t ChoixLed)

  R�sum� :
    Contr�le l'�tat des LEDs en fonction du masque de choix sp�cifi�.

  Description :
    Cette fonction prend en entr�e un masque de bits (ChoixLed) o� chaque bit
    correspond � l'�tat (allum� ou �teint) d'une LED sp�cifique. Les bits � 1
    indiquent d'allumer la LED correspondante, tandis que les bits � 0 indiquent
    de l'�teindre. La fonction utilise ce masque pour contr�ler l'�tat de chaque
    LED individuellement.

  Remarques :
    Les LEDs sont identifi�es de BSP_LED_0 � BSP_LED_7.
    Le masque ChoixLed permet de s�lectionner quelles LEDs doivent �tre allum�es.
    Les LEDs non s�lectionn�es sont �teintes.

    Exemple :
    - Pour allumer BSP_LED_0 et BSP_LED_3, ChoixLed = 0x09 (00001001 en binaire).
    - Pour �teindre toutes les LEDs, ChoixLed = 0x00.
    - Pour allumer toutes les LEDs, ChoixLed = 0xFF.
*/
// *****************************************************************************
void EteindreLEDS(void)
{
    BSP_LEDOff(BSP_LED_0);
    BSP_LEDOff(BSP_LED_1);
    BSP_LEDOff(BSP_LED_2);
    BSP_LEDOff(BSP_LED_3);
    BSP_LEDOff(BSP_LED_4);
    BSP_LEDOff(BSP_LED_5);
    BSP_LEDOff(BSP_LED_6);
    BSP_LEDOff(BSP_LED_7);
}




// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;
    
    
    /* TODO: Initialize your application's state machine and other
     * parameters.
     */
}


/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks(void)
{
    /* V�rifier l'�tat actuel de l'application. */
    switch (appData.state)
    {
        /* �tat initial de l'application. */
        case APP_STATE_INIT:
        {
            static int8_t compteurInit = 0;
            
            if(compteurInit == 0)
            {
                // Initialisation de l'afficheur LCD
                lcd_init();
                lcd_bl_on();
                
                // Initialisation du convertisseur analogique-num�rique
                BSP_InitADC10Alt();
             
                // �teint toutes les LEDs
                EteindreLEDS();

                // Affiche des informations sur l'afficheur LCD
                lcd_gotoxy(1,1);
                printf_lcd("Local Settings");
                lcd_gotoxy(1,2);
                printf_lcd("TP2 PWM_RS232 23-24");
                lcd_gotoxy(1,3);
                printf_lcd("Cyril Feliciano");
        
                // Initialise le g�n�rateur de PWM
                GPWM_Initialize(&pData);
                // Initialise la Fifo
                InitFifoComm();
                // Incr�mente le compteur d'initialisation pour n'ex�cuter ces �tapes qu'une seule fois
                compteurInit++;
            }
            
            // Mettre � jour l'�tat du switch
            APP_UpdateState(APP_STATE_WAIT);
            break;
        }

        case APP_STATE_SERVICE_TASKS:
        {
            static uint8_t CommStatus = 0;
            static uint8_t comptSend = 0;
                     
            // R�ception param. remote
            CommStatus = GetMessage(&PWMData);
            if (CommStatus == 0) // Si c'est local.
            {
              GPWM_GetSettings(&PWMData); // Obtient les param�tres locaux.
            }
            else
            {
              GPWM_GetSettings(&PWMDataToSend); // Obtient les param�tres � distance.
            }

            // Affichage des param�tres sur un �cran.
            GPWM_DispSettings(&PWMData, CommStatus);

            // Ex�cution du PWM et gestion du moteur en utilisant les param�tres obtenus.
            GPWM_ExecPWM(&PWMData);

            // Envoi p�riodique des donn�es si n�cessaire.
            if(comptSend >= 5) // Si le compteur d'envoi atteint 5.
            {
                if (CommStatus == 0) // Si c'est local.
                {
                    SendMessage(&PWMData); // Envoie les donn�es locales.
                }    
                else
                {
                    SendMessage(&PWMDataToSend); // Envoie les donn�es � distance.
                }
                comptSend = 0; // R�initialisation du compteur d'envoi.
            }
            else
            {
                comptSend++; // Incr�mentation du compteur d'envoi.
            }
            
            appData.state = APP_STATE_WAIT; // Changement d'�tat de l'application vers l'�tat d'attente.

            break;
            
        }
        
 
        case APP_STATE_WAIT:
        {
            // �tat d'attente, aucune action particuli�re ici
            break;
        }

        /* L'�tat par d�faut ne devrait jamais �tre ex�cut�. */
        default:
        {
            break;
        }
    }
}





/*******************************************************************************
 End of File
 */
