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
#include "gestPWM.h"

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

  Résumé :
    Fonction de rappel du timer 1.

  Description :
    Cette fonction est appelée à chaque déclenchement du timer 1, qui est
    configuré pour des déclenchements tous les 100 ms. Elle maintient un
    compteur statique, et lorsqu'il atteint la valeur 150 (3 secondes),
    elle déclenche un changement d'état de l'application vers APP_STATE_SERVICE_TASKS
    en appelant la fonction APP_UpdateState. Elle est réappellée toutes les 100ms 

  Remarques :
    - La fonction est associée à un timer configuré pour des
      déclenchements périodiques tous les 100 ms.

*/
// *****************************************************************************
void callback_timer1(void)
{
    static uint8_t compteur3s = 0;
        // Déclencher un changement d'état vers APP_STATE_SERVICE_TASKS
        if (compteur3s <= COMPTEUR_3_SECONDES)
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

  Résumé :
    Met à jour l'état global de l'application.

  Description :
    Cette fonction met à jour l'état global du switch avec la nouvelle
    valeur spécifiée en tant que paramètre. Elle doit être appelée pour changer
    l'état de l'application selon les besoins de la logique de la machine à états.

  Remarques :
    La variable globale appData.state est mise à jour directement avec la nouvelle
    valeur spécifiée en paramètre. Aucune valeur de retour n'est fournie, car la
    modification est effectuée directement sur la variable globale d'état.

    Cette fonction est utilisée pour faciliter la gestion de l'état de l'application
    dans la machine à états principale de l'application.
*/
// *****************************************************************************
void APP_UpdateState(APP_STATES NewState)
{
    // Met à jour l'état de l'application avec la nouvelle valeur
    appData.state = NewState;
    
    // Aucune sortie explicite, car la mise à jour est effectuée directement sur la variable d'état globale.
    // La fonction n'a pas de valeur de retour (void).
}


// *****************************************************************************

// *****************************************************************************
/* Fonction :
    void Allume_Leds(uint8_t ChoixLed)

  Résumé :
    Contrôle l'état des LEDs en fonction du masque de choix spécifié.

  Description :
    Cette fonction prend en entrée un masque de bits (ChoixLed) où chaque bit
    correspond à l'état (allumé ou éteint) d'une LED spécifique. Les bits à 1
    indiquent d'allumer la LED correspondante, tandis que les bits à 0 indiquent
    de l'éteindre. La fonction utilise ce masque pour contrôler l'état de chaque
    LED individuellement.

  Remarques :
    Les LEDs sont identifiées de BSP_LED_0 à BSP_LED_7.
    Le masque ChoixLed permet de sélectionner quelles LEDs doivent être allumées.
    Les LEDs non sélectionnées sont éteintes.

    Exemple :
    - Pour allumer BSP_LED_0 et BSP_LED_3, ChoixLed = 0x09 (00001001 en binaire).
    - Pour éteindre toutes les LEDs, ChoixLed = 0x00.
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
      static uint8_t CommStatus = 0;
      static uint8_t comptSend = 0;
    /* Vérifier l'état actuel de l'application. */
    switch (appData.state)
    {
        /* État initial de l'application. */
        case APP_STATE_INIT:
        {
            static int8_t compteurInit = 0;
            
            if(compteurInit == 0)
            {
                // Initialisation de l'afficheur LCD
                lcd_init();
                lcd_bl_on();
                
                // Initialisation du convertisseur analogique-numérique
                BSP_InitADC10Alt();
             
                // Éteint toutes les LEDs
                EteindreLEDS();

                // Affiche des informations sur l'afficheur LCD
                lcd_gotoxy(1,1);
                printf_lcd("Local Settings");
                lcd_gotoxy(1,2);
                printf_lcd("TP2 PWM_RS232 23-24");
                lcd_gotoxy(1,3);
                printf_lcd("Cyril Feliciano");
        
                // Initialise le générateur de PWM
                GPWM_Initialize(&pData);
                // Initialise la Fifo
                InitFifoComm();
                // Incrémente le compteur d'initialisation pour n'exécuter ces étapes qu'une seule fois
                compteurInit++;
            }
            
            // Mettre à jour l'état du switch
            APP_UpdateState(APP_STATE_WAIT);
            break;
        }

        case APP_STATE_SERVICE_TASKS:
        {

                     
            // Réception param. remote
            CommStatus = GetMessage(&PWMData);
            if (CommStatus == 0) // Si c'est local.
            {
              GPWM_GetSettings(&PWMData); // Obtient les paramètres locaux.
            }
            else
            {
              GPWM_GetSettings(&PWMDataToSend); // Obtient les paramètres à distance.
            }

            // Affichage des paramètres sur un écran.
            GPWM_DispSettings(&PWMData, CommStatus);

            // Exécution du PWM et gestion du moteur en utilisant les paramètres obtenus.
            GPWM_ExecPWM(&PWMData);

            // Envoi périodique des données si nécessaire.
            if(comptSend >= 5) // Si le compteur d'envoi atteint 5.
            {
                if (CommStatus == 0) // Si c'est local.
                {
                    SendMessage(&PWMData); // Envoie les données locales.
                }    
                else
                {
                    SendMessage(&PWMDataToSend); // Envoie les données à distance.
                }
                comptSend = 0; // Réinitialisation du compteur d'envoi.
            }
            else
            {
                comptSend++; // Incrémentation du compteur d'envoi.
            }
            
            appData.state = APP_STATE_WAIT; // Changement d'état de l'application vers l'état d'attente.

            break;
            
        }
        
 
        case APP_STATE_WAIT:
        {
            // État d'attente, aucune action particulière ici
            break;
        }

        /* L'état par défaut ne devrait jamais être exécuté. */
        default:
        {
            break;
        }
    }
}





/*******************************************************************************
 End of File
 */
