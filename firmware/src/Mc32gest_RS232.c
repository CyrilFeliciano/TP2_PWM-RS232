// Mc32Gest_RS232.C
// Canevas manipulatio TP2 RS232 SLO2 2017-18
// Fonctions d'émission et de réception des message
// CHR 20.12.2016 ajout traitement int error
// CHR 22.12.2016 evolution des marquers observation int Usart
// SCA 03.01.2018 nettoyé réponse interrupt pour ne laisser que les 3 ifs

#include <xc.h>
#include <sys/attribs.h>
#include "system_definitions.h"
// Ajout CHR
#include <GenericTypeDefs.h>
#include "app.h"
#include "GesFifoTh32.h"
#include "Mc32gest_RS232.h"
#include "gestPWM.h"
#include "Mc32CalCrc16.h"
#include <stdint.h>
#include <stdbool.h>



typedef union {
        uint16_t val;
        struct {uint8_t lsb;
                uint8_t msb;} shl;
} U_manip16;


// Definition pour les messages
#define MESS_SIZE  5
// avec int8_t besoin -86 au lieu de 0xAA
#define STX_code  (-86)


// Structure décrivant le message
typedef struct {
    int8_t Start;
    int8_t  Speed;
    int8_t  Angle;
    int8_t MsbCrc;
    int8_t LsbCrc;
} StruMess;


// Struct pour émission des messages
StruMess TxMess;
// Struct pour réception des messages
StruMess RxMess;


// Declaration des FIFO pour réception et émission
#define FIFO_RX_SIZE ( (4*MESS_SIZE) + 1)  // 4 messages
#define FIFO_TX_SIZE ( (4*MESS_SIZE) + 1)  // 4 messages

int8_t fifoRX[FIFO_RX_SIZE];
// Declaration du descripteur du FIFO de réception
S_fifo descrFifoRX;


int8_t fifoTX[FIFO_TX_SIZE];
// Declaration du descripteur du FIFO d'émission
S_fifo descrFifoTX;


// Initialisation de la communication sérielle
void InitFifoComm(void)
{    
    // Initialisation du fifo de réception
    InitFifo ( &descrFifoRX, FIFO_RX_SIZE, fifoRX, 0 );
    // Initialisation du fifo d'émission
    InitFifo ( &descrFifoTX, FIFO_TX_SIZE, fifoTX, 0 );
    
    // Init RTS 
    RS232_RTS = 1;   // interdit émission par l'autre
   
} // InitComm



/******************************************************************************
    Auteur : CFO
 *
    Fonction :
    int GetMessage(S_pwmSettings *pData)
 * 
    Résumé :
    Cette fonction gère la réception de messages via l'interface de communication.
    Elle récupère les données du FIFO de réception et vérifie l'intégrité des messages
    en calculant et en vérifiant les valeurs de CRC16. La fonction ajuste également
    l'état de la broche de demande de transmission (RTS) pour gérer le contrôle
    de flux de la réception.
 * 
    Paramètres :
    pData : Un pointeur vers la structure de paramètres PWM (S_pwmSettings) où seront
    stockées les données de vitesse et d'angle reçues.
 * 
    Retour :
    La fonction retourne le statut de la communication, indiquant si le message reçu
    était valide ou non.
******************************************************************************/

// Valeur de retour 0  = pas de message reçu donc local (data non modifié)
// Valeur de retour 1  = message reçu donc en remote (data mis à jour)
int GetMessage(S_pwmSettings *pData)
{
    // Traitement de réception à introduire ICI
    // Lecture et décodage fifo réception
    // ...
    uint8_t NbCharToRead = 0;
    static uint8_t NbrCycle = 0;
    static uint8_t CommStatus = 0;
    uint32_t ValCRC = 0xFFFF;
    U_manip16 CRC16;
    
    // Obtient le nombre de caractères à lire depuis le FIFO de réception.
    NbCharToRead = GetReadSize(&descrFifoRX);
    
    // Obtient le caractère de début du message depuis le FIFO de réception.
    GetCharFromFifo(&descrFifoRX, &RxMess.Start);

    // Obtention du nombre d'octets dans le FIFO et mettre cette valeur dans NbCharToRead 
    // Vérifie si suffisamment de caractères ont été reçus et si le caractère de début est correct.
    if ((NbCharToRead >= MESS_SIZE) && (RxMess.Start == STX_code))
    {
        // Obtient les caractères de vitesse, d'angle et de CRC depuis le FIFO de réception.
        GetCharFromFifo(&descrFifoRX, &RxMess.Speed);
        GetCharFromFifo(&descrFifoRX, &RxMess.Angle);	 
        GetCharFromFifo(&descrFifoRX, &RxMess.MsbCrc);
        GetCharFromFifo(&descrFifoRX, &RxMess.LsbCrc);

        // Calcule la valeur du CRC16 à partir des octets reçus.
        CRC16.shl.lsb = RxMess.LsbCrc;
        CRC16.shl.msb = RxMess.MsbCrc;
        

        // Met à jour la valeur CRC16 en ajoutant les octets du message.
        ValCRC = updateCRC16(ValCRC, RxMess.Start);
        ValCRC = updateCRC16(ValCRC, RxMess.Speed);
        ValCRC = updateCRC16(ValCRC, RxMess.Angle);

        // Vérifie si le CRC16 calculé correspond au CRC16 reçu.
        if (ValCRC == CRC16.val)
        {   
            // Met à jour les paramètres de l'angle et de la vitesse avec les valeurs reçues.
            pData->AngleSetting = RxMess.Angle;
            pData->SpeedSetting = RxMess.Speed;

            // Calcule la valeur absolue de la vitesse.
            if(pData->SpeedSetting < 0)
            {
                pData->absSpeed = RxMess.Speed * -1;
            }
            else
            {
                pData->absSpeed = RxMess.Speed;
            }
            // Réinitialise le nombre de cycles et le statut de la communication.
            NbrCycle = 0;
            CommStatus = 1;
        }
        
        // Si la CRC16 ne correspond pas et si le nombre de cycles n'a pas atteint 10, incrémente le nombre de cycles.
        else if (NbrCycle < CYCLE_MAX)
        {
            BSP_LEDToggle(BSP_LED_6);
            NbrCycle++;
        }
    }    
    // Si le nombre de cycles n'a pas atteint 10 et si le message est incomplet, incrémente le nombre de cycles.
    else
    {
        if (NbrCycle < CYCLE_MAX)
        {
            NbrCycle++;
        }
        // Si le nombre de cycles atteint 10, réinitialise le statut de la communication et le nombre de cycles.
        else
        {
            CommStatus = 0;
            NbrCycle = 0;
        }
    }
    // Gestion controle de flux de la réception
    if(GetWriteSpace ( &descrFifoRX) >= (2*MESS_SIZE))
    {
        // autorise émission par l'autre
        RS232_RTS = 0;
    }
    return CommStatus;
} // GetMessage


/******************************************************************************
    Auteur : CFO
 *
    Fonction :
    void SendMessage(S_pwmSettings *pData)
 * 
    Résumé :
    Cette fonction prépare et envoie un message via l'interface de communication,
    en utilisant les informations contenues dans la structure de paramètres PWM.
 * 
    Description :
    La fonction calcule un nouveau CRC16 basé sur les paramètres de vitesse et d'angle de la structure pData.
    Elle stocke ensuite ce CRC16 avec les données dans une structure TxMess.
    Si l'espace est disponible dans le FIFO de transmission, elle y écrit les données.
    De plus, elle gère le contrôle de flux en activant une interruption de transmission
    lorsque la broche de demande de transmission (CTS) est désactivée et qu'il y a de l'espace disponible dans le FIFO.
 *
    Paramètres :
    pData : Un pointeur vers la structure de paramètres PWM (S_pwmSettings)
*/

// Fonction d'envoi des messages, appel cyclique
void SendMessage(S_pwmSettings *pData)
{
    int8_t freeSize;
    uint8_t TransmitCRCHigh = 0;
    uint8_t TransmitCRCLow = 0;
    uint32_t NewCRC = 0xFFFF;
    // Traitement émission à introduire ICI
    // Formatage message et remplissage fifo émission
    // Obtient l'espace disponible dans le FIFO de transmission.
    freeSize = GetWriteSpace(&descrFifoTX);

    // Vérifie si suffisamment d'espace est disponible dans le FIFO pour écrire le message complet.
    if(freeSize >= MESS_SIZE)
    {
        // Calcule le nouveau CRC16 à partir des paramètres de vitesse et d'angle.
        NewCRC = updateCRC16(NewCRC, STX_code);
        NewCRC = updateCRC16(NewCRC, pData->SpeedSetting);
        NewCRC = updateCRC16(NewCRC, pData->AngleSetting);

        // Sépare le CRC16 en octets MSB et LSB.
        TransmitCRCHigh = (NewCRC & 0xFF00) >> 8;
        TransmitCRCLow = NewCRC & 0x00FF;

        // Remplit la structure TxMess avec les paramètres et le CRC16.
        TxMess.Start = STX_code;
        TxMess.Speed = pData->SpeedSetting;
        TxMess.Angle = pData->AngleSetting;
        TxMess.MsbCrc = TransmitCRCHigh;
        TxMess.LsbCrc = TransmitCRCLow;

        // Écrit chaque élément de la structure TxMess dans le FIFO de transmission.
        PutCharInFifo(&descrFifoTX, TxMess.Start);
        PutCharInFifo(&descrFifoTX, TxMess.Speed);
        PutCharInFifo(&descrFifoTX, TxMess.Angle);
        PutCharInFifo(&descrFifoTX, TxMess.MsbCrc);
        PutCharInFifo(&descrFifoTX, TxMess.LsbCrc);
    }    
    // Gestion du controle de flux
    // si on a un caractère à envoyer et que CTS = 0
    freeSize = GetWriteSpace(&descrFifoTX);
    if ((RS232_CTS == 0) && (freeSize > 0))
    {        
        // Autorise int émission    
        PLIB_INT_SourceEnable(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);                
    }
}


// Interruption USART1
// !!!!!!!!
// Attention ne pas oublier de supprimer la réponse générée dans system_interrupt
// !!!!!!!!
void __ISR(_UART_1_VECTOR, ipl5AUTO) _IntHandlerDrvUsartInstance0(void)
{    
    uint8_t dataAvaliable = 0;
    uint8_t freeSize, TXSize;
    uint8_t byteUsart = 0;
    int8_t c;
    int8_t i_cts = 0;
    bool TxBuffFull;
    USART_ERROR UsartStatus;
 
    // Marque début interruption avec Led3
    LED3_W = 1;
    // Is this an Error interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_ERROR) && PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_ERROR) )
    {
        /* Clear pending interrupt */
        PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_ERROR);
        // Traitement de l'erreur à la réception.
        while (PLIB_USART_ReceiverDataIsAvailable(USART_ID_1))
        {
            PLIB_USART_ReceiverByteReceive(USART_ID_1);
        }
    }
 
    // Is this an RX interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_RECEIVE) && PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_RECEIVE) ) 
    {
                
 
        // Oui Test si erreur parité ou overrun
        UsartStatus = PLIB_USART_ErrorsGet(USART_ID_1);
 
        if ( (UsartStatus & (USART_ERROR_PARITY | USART_ERROR_FRAMING | USART_ERROR_RECEIVER_OVERRUN)) == 0)
        {
 
            // Traitement RX à faire ICI
            // Lecture des caractères depuis le buffer HW -> fifo SW
			//  (pour savoir s'il y a une data dans le buffer HW RX : PLIB_USART_ReceiverDataIsAvailable())
			//  (Lecture via fonction PLIB_USART_ReceiverByteReceive())
            // ...


            dataAvaliable = PLIB_USART_ReceiverDataIsAvailable(USART_ID_1);
            
            if(dataAvaliable == 1)
            {
                byteUsart = PLIB_USART_ReceiverByteReceive(USART_ID_1);    
                PutCharInFifo(&descrFifoRX, byteUsart);
            }         
            LED4_W = !LED4_R; // Toggle Led4
            // buffer is empty, clear interrupt flag
            PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_RECEIVE);
        }
        else 
        {
            // Suppression des erreurs
            // La lecture des erreurs les efface sauf pour overrun
            if ( (UsartStatus & USART_ERROR_RECEIVER_OVERRUN) == USART_ERROR_RECEIVER_OVERRUN)
            {
                   PLIB_USART_ReceiverOverrunErrorClear(USART_ID_1);
            }
        }
 
        
        // Traitement controle de flux reception à faire ICI
        // Gerer sortie RS232_RTS en fonction de place dispo dans fifo reception
        // ...
        freeSize = GetWriteSpace(&descrFifoRX);
        if (freeSize <= 6){
            //controle de flux : demande stop émission
            RS232_RTS = 1 ;
        }        
    } // end if RX
    
    // Is this an TX interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT) && PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT) )        
    {
 
        // Traitement TX à faire ICI
        // Envoi des caractères depuis le fifo SW -> buffer HW
        TXSize = GetReadSize (&descrFifoTX);
        i_cts = RS232_CTS;
        // Avant d'émettre, on vérifie 3 conditions :
        //  Si CTS = 0 autorisation d'émettre (entrée RS232_CTS)
        //  S'il y a un caratères à émettre dans le fifo
        //  S'il y a de la place dans le buffer d'émission (PLIB_USART_TransmitterBufferIsFull)
        //   (envoi avec PLIB_USART_TransmitterByteSend())
        // ...
         TXSize = GetReadSize (&descrFifoTX);
         TxBuffFull = PLIB_USART_TransmitterBufferIsFull(USART_ID_1);
         
         if ( (i_cts == 0) && ( TXSize > 0 ) && TxBuffFull == false )
         { 
            do {
              GetCharFromFifo(&descrFifoTX, &c);
              PLIB_USART_TransmitterByteSend(USART_ID_1, c);
              BSP_LEDToggle(BSP_LED_6); // pour comptage
              i_cts = RS232_CTS;
              TXSize = GetReadSize (&descrFifoTX);
              TxBuffFull = PLIB_USART_TransmitterBufferIsFull(USART_ID_1);
            } while ( (i_cts == 0) && ( TXSize > 0 ) && TxBuffFull == false );
            // Clear the TX interrupt Flag
            // (Seulement aprés TX)

            TXSize = GetReadSize (&descrFifoTX);

            if (TXSize == 0 )
            {
            // pour éviter une interruption inutile
            PLIB_INT_SourceDisable(INT_ID_0,INT_SOURCE_USART_1_TRANSMIT);
            }
         }
         else
         {
            // disable TX interrupt
            PLIB_INT_SourceDisable(INT_ID_0,INT_SOURCE_USART_1_TRANSMIT);

            PLIB_INT_SourceFlagClear(INT_ID_0,INT_SOURCE_USART_1_TRANSMIT);
         }
    }
        LED5_W = !LED5_R; // Toggle Led5
        // disable TX interrupt (pour éviter une interrupt. inutile si plus rien à transmettre)
        PLIB_INT_SourceDisable(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);
        
         // Clear the TX interrupt Flag (Seulement apres TX) 
         PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);
 
    // Marque fin interruption avec Led3
    LED3_W = 0;
}

 

 




