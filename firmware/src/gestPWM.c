/*--------------------------------------------------------*/
// GestPWM.c
/*--------------------------------------------------------*/
//	Description :	Gestion des PWM 
//			        pour TP1 2016-2017
//
//	Auteur 		: 	C. HUBER
//
//	Version		:	V1.1
//	Compilateur	:	XC32 V1.42 + Harmony 1.08
//
/*--------------------------------------------------------*/

#include "GestPWM.h"
#include <stdint.h>
#include <math.h>

// *****************************************************************************
/* Fonction :
    void GPWM_Initialize(S_pwmSettings *pData)

  Résumé :
    Initialise la structure de paramètres PWM et configure le matériel associé.

  Description :
    Cette fonction initialise la structure de données S_pwmSettings pointée par
    pData en mettant à zéro ses membres relatifs à l'angle, à la vitesse, et à
    leurs valeurs absolues. Ensuite, elle configure l'état du pont en H à l'aide
    de la fonction BSP_EnableHbrige(). Enfin, elle démarre les timers et les
    sorties de comparaison (OC - Output Compare) nécessaires pour la génération
    de signaux PWM en appelant les fonctions appropriées de la bibliothèque DRV.

  Paramètres :
    - pData : Un pointeur vers la structure de paramètres PWM (S_pwmSettings).

   
*/
// *****************************************************************************

void GPWM_Initialize(S_pwmSettings *pData)
{
    // Initialise les données de la structure pData
    pData->AngleSetting = 0;
    pData->SpeedSetting = 0;
    pData->absAngle = 0;
    pData->absSpeed = 0;
    
    // Initialise l'état du pont en H
    BSP_EnableHbrige();
    
    // Lance les timers et les sorties de comparaison (OC - Output Compare)
    DRV_TMR0_Start();
    DRV_TMR1_Start();
    DRV_TMR2_Start();
    
    DRV_OC0_Start();
    DRV_OC1_Start();
}

// *****************************************************************************
/* Fonction :
    void GPWM_GetSettings(S_pwmSettings *pData)

  Résumé :
    Obtient les valeurs de vitesse et d'angle à partir du convertisseur AD.

  Description :
    Cette fonction récupère les valeurs de vitesse et d'angle à partir du
    convertisseur analogique-numérique (AD). Elle lit les valeurs du canal 0
    (vitesse) et du canal 1 (angle) du convertisseur AD, effectue une moyenne
    sur un certain nombre d'échantillons (défini par TAILLE_MOYENNE_ADC) pour
    réduire les variations du signal, puis effectue une conversion en unités
    appropriées.

  Paramètres :
    - pData : Un pointeur vers la structure de paramètres PWM (S_pwmSettings)
              où seront stockées les valeurs de vitesse et d'angle.

*/
// *****************************************************************************

void GPWM_GetSettings(S_pwmSettings *pData)	
{
    // Lecture du convertisseur AD
    static uint32_t valeur_ADC1[TAILLE_MOYENNE_ADC] = {0};
    static uint32_t valeur_ADC2[TAILLE_MOYENNE_ADC] = {0};
    static uint32_t i = 0;
    uint8_t n;
    uint32_t somme1 = 0;
    uint32_t somme2 = 0;
    uint32_t moyen_ADC1, moyen_ADC2;
    int32_t valeur_variant_ADC1, valeur_variant_ADC2;
    APP_DATA appData;

    // Lire les valeurs du convertisseur analogique-numérique
    appData.AdcRes = BSP_ReadADCAlt();
    valeur_ADC1[i] = appData.AdcRes.Chan0;
    valeur_ADC2[i] = appData.AdcRes.Chan1;
    i++;
    if (i > 9)
    {
        i = 0;
    }

    // Calculer la moyenne des échantillons pour lisser le signal
    for (n = 0; n < TAILLE_MOYENNE_ADC; n++)
    {
        somme1 += valeur_ADC1[n];
        somme2 += valeur_ADC2[n];
    }
    moyen_ADC1 = somme1 / TAILLE_MOYENNE_ADC;
    moyen_ADC2 = somme2 / TAILLE_MOYENNE_ADC;

    // Conversion des valeurs ADC en unités appropriées
    valeur_variant_ADC1 = ((198 * moyen_ADC1) / 1023) + 0.5;
    valeur_variant_ADC1 = valeur_variant_ADC1 - 99;
    valeur_variant_ADC2 = ((180 * moyen_ADC2) / 1023) + 0.5;

    // Stockage des valeurs converties dans la structure de paramètres
    pData->absAngle = valeur_variant_ADC2;
    valeur_variant_ADC2 = (valeur_variant_ADC2 - 90);
    pData->AngleSetting = valeur_variant_ADC2;
    pData->SpeedSetting = valeur_variant_ADC1;
    pData->absSpeed = abs(valeur_variant_ADC1);
}



// *****************************************************************************
/* Fonction :
    void GPWM_DispSettings(S_pwmSettings *pData)

  Résumé :
    Affiche les informations de la structure de paramètres sur l'afficheur LCD.

  Description :
    Cette fonction prend en entrée une structure de paramètres PWM (pData) et
    affiche les informations de vitesse (SpeedSetting, absSpeed) et d'angle
    (AngleSetting) sur un afficheur LCD. Les valeurs sont formatées pour
    afficher le signe et la valeur absolue des paramètres.
    Elle gère egalement l'affichage du mode local et remote.

  Paramètres :
    - pData : Un pointeur vers la structure de paramètres PWM (S_pwmSettings)
              contenant les informations de vitesse et d'angle.
 *  - Remote : Variable qui me permet de savoir si je suis en mode local ou remote.

*/
// *****************************************************************************
void GPWM_DispSettings(S_pwmSettings *pData, int Remote)
{
    static uint8_t compteurAffichageLcd = 0;
    static uint8_t compteurClearLine;
    static uint8_t oldRemoteValue = 0;
    
    // Si les lignes du LCD n'ont pas été effacées auparavant (compteurAffichageLcd est à 0), efface les lignes et affiche les titres.
    if(compteurAffichageLcd == 0)
    {
        for(compteurClearLine = 1; compteurClearLine < CINQUE; compteurClearLine++)
        {
             lcd_ClearLine(compteurClearLine);
        }
        
        lcd_gotoxy(1,2); // Déplacement du curseur à la position (1,2) pour afficher "SpeedSetting".
        printf_lcd("SpeedSetting");
        lcd_gotoxy(1,3); // Déplacement du curseur à la position (1,3) pour afficher "absSpeed".
        printf_lcd("absSpeed");
        lcd_gotoxy(1,4); // Déplacement du curseur à la position (1,4) pour afficher "Angle".
        printf_lcd("Angle");
        compteurAffichageLcd++; // Incrémente le compteur pour indiquer que les lignes ont été effacées.
    }
 
    // Affiche "Local Settings" ou "** Remote Settings" sur la première ligne du LCD en fonction de la valeur de Remote.
    if(oldRemoteValue != Remote)
    {
        if(Remote == 0)
        {   
            lcd_gotoxy(1,1); // Déplacement du curseur à la position (1,1) pour afficher le statut des réglages.
            printf_lcd("Local Settings    ");
        }
        else
        {
            lcd_gotoxy(1,1); // Déplacement du curseur à la position (1,1) pour afficher le statut des réglages.
            printf_lcd("** Remote Settings");
        }
    }
        // Affiche les valeurs de vitesse, de vitesse absolue et d'angle sur le LCD.
        lcd_gotoxy(14,2); // Déplacement du curseur à la position (14,2) pour afficher la vitesse de réglage.
        printf_lcd("%3d ", pData->SpeedSetting);
        lcd_gotoxy(15,3); // Déplacement du curseur à la position (15,3) pour afficher la vitesse absolue.
        printf_lcd("%2d ", pData->absSpeed);
        lcd_gotoxy(14,4); // Déplacement du curseur à la position (14,4) pour afficher l'angle de réglage.
        printf_lcd("%3d ", pData->AngleSetting);

    oldRemoteValue = Remote;
}

     

    




// *****************************************************************************
/* Fonction :
    void GPWM_ExecPWM(S_pwmSettings *pData)

  Résumé :
    Exécute la modulation de largeur d'impulsion (PWM) et gère le moteur en
    fonction des informations contenues dans la structure de paramètres.

  Description :
    Cette fonction prend en entrée une structure de paramètres PWM (pData)
    contenant les informations sur la vitesse (SpeedSetting) et l'angle
    (AngleSetting). En fonction de la vitesse, elle contrôle l'état du pont en H
    pour définir la direction du moteur. Ensuite, elle calcule la largeur
    d'impulsion (Pulse Width) en utilisant les informations de vitesse et d'angle
    pour les sorties de comparaison OC2 et OC3. Elle utilise la bibliothèque PLIB
    pour effectuer ces opérations.

  Paramètres :
    - pData : Un pointeur vers la structure de paramètres PWM (S_pwmSettings)
              contenant les informations de vitesse et d'angle.

*/
// *****************************************************************************
void GPWM_ExecPWM(S_pwmSettings *pData)
{
    static uint16_t PulseWidthOC2;
    static uint16_t PulseWidthOC3;

    // Contrôle de l'état du pont en H en fonction de la vitesse
    if (pData->SpeedSetting < 0)
    {
        PLIB_PORTS_PinSet(PORTS_ID_0, AIN1_HBRIDGE_PORT, AIN1_HBRIDGE_BIT);
        PLIB_PORTS_PinClear(PORTS_ID_0, AIN2_HBRIDGE_PORT, AIN2_HBRIDGE_BIT);
    }
    else if (pData->SpeedSetting > 0)
    {
        PLIB_PORTS_PinClear(PORTS_ID_0, AIN1_HBRIDGE_PORT, AIN1_HBRIDGE_BIT);
        PLIB_PORTS_PinSet(PORTS_ID_0, AIN2_HBRIDGE_PORT, AIN2_HBRIDGE_BIT);
    }
    else
    {
        PLIB_PORTS_PinClear(PORTS_ID_0, AIN1_HBRIDGE_PORT, AIN1_HBRIDGE_BIT);
        PLIB_PORTS_PinClear(PORTS_ID_0, AIN2_HBRIDGE_PORT, AIN2_HBRIDGE_BIT);
    }

    // Calcul de la largeur d'impulsion (Pulse Width) pour la sortie OC2 en fonction de la vitesse
    PulseWidthOC2 = (abs(pData->SpeedSetting) * DRV_TMR1_PeriodValueGet()) / 100;
    PLIB_OC_PulseWidth16BitSet(OC_ID_2, PulseWidthOC2);

    // Calcul de la largeur d'impulsion (Pulse Width) pour la sortie OC3 en fonction de l'angle
    PulseWidthOC3 = (((pData->AngleSetting +90) * 9000) / 180) + 2999;
    PLIB_OC_PulseWidth16BitSet(OC_ID_3, PulseWidthOC3);
}
 
    

// *****************************************************************************
/* Fonction :
    void GPWM_ExecPWMSoft(S_pwmSettings *pData)

  Résumé :
    Exécute la modulation de largeur d'impulsion (PWM) logiciel.

  Description :
    Cette fonction prend en entrée une structure de paramètres PWM (pData)
    contenant les informations sur la vitesse (SpeedSetting). Elle utilise une
    méthode logicielle pour simuler une modulation de largeur d'impulsion. La LED
    BSP_LED_2 est éteinte si la valeur absolue de la vitesse est supérieure au
    compteur compteurRpc, sinon elle est allumée. Le compteurRpc est incrémenté
    à chaque appel de la fonction et remis à zéro s'il dépasse la valeur 99.

  Paramètres :
    - pData : Un pointeur vers la structure de paramètres PWM (S_pwmSettings)
              contenant les informations de vitesse.

*/
// *****************************************************************************
void GPWM_ExecPWMSoft(S_pwmSettings *pData)
{
    static uint8_t compteurRpc = 0;

    // Si la valeur absolue de la vitesse est supérieure au compteurRpc, éteindre la LED
    if (pData->absSpeed > compteurRpc)
    {
        BSP_LEDOff(BSP_LED_2);
    }
    else
    {
        // Sinon, allumer la LED
        BSP_LEDOn(BSP_LED_2);
    }

    // Incrémente le compteurRpc
    compteurRpc++;

    // Remet à zéro le compteurRpc s'il dépasse la valeur 99
    if (compteurRpc > 99)
    {
        compteurRpc = 0;
    }
}

