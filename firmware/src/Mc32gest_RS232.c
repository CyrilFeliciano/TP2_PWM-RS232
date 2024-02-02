// Mc32Gest_RS232.C
// Canevas manipulatio TP2 RS232 SLO2 2017-18
// Fonctions d'�mission et de r�ception des message
// CHR 20.12.2016 ajout traitement int error
// CHR 22.12.2016 evolution des marquers observation int Usart
// SCA 03.01.2018 nettoy� r�ponse interrupt pour ne laisser que les 3 ifs

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



typedef union {
        uint16_t val;
        struct {uint8_t lsb;
                uint8_t msb;} shl;
} U_manip16;


// Definition pour les messages
#define MESS_SIZE  5
// avec int8_t besoin -86 au lieu de 0xAA
#define STX_code  (-86)
// Nombre de cycle maximal pour compteur NbrCycle
#define CYCLE_MAX 9

// Structure d�crivant le message
typedef struct {
    int8_t Start;
    int8_t  Speed;
    int8_t  Angle;
    int8_t MsbCrc;
    int8_t LsbCrc;
} StruMess;


// Struct pour �mission des messages
StruMess TxMess;
// Struct pour r�ception des messages
StruMess RxMess;


// Declaration des FIFO pour r�ception et �mission
#define FIFO_RX_SIZE ( (4*MESS_SIZE) + 1)  // 4 messages
#define FIFO_TX_SIZE ( (4*MESS_SIZE) + 1)  // 4 messages

int8_t fifoRX[FIFO_RX_SIZE];
// Declaration du descripteur du FIFO de r�ception
S_fifo descrFifoRX;


int8_t fifoTX[FIFO_TX_SIZE];
// Declaration du descripteur du FIFO d'�mission
S_fifo descrFifoTX;


// Initialisation de la communication s�rielle
void InitFifoComm(void)
{    
    // Initialisation du fifo de r�ception
    InitFifo ( &descrFifoRX, FIFO_RX_SIZE, fifoRX, 0 );
    // Initialisation du fifo d'�mission
    InitFifo ( &descrFifoTX, FIFO_TX_SIZE, fifoTX, 0 );
    
    // Init RTS 
    RS232_RTS = 1;   // interdit �mission par l'autre
   
} // InitComm



/******************************************************************************
    Auteur : CFO
 *
    Fonction :
    int GetMessage(S_pwmSettings *pData)
 * 
    R�sum� :
    Cette fonction g�re la r�ception de messages via l'interface de communication.
    Elle r�cup�re les donn�es du FIFO de r�ception et v�rifie l'int�grit� des messages
    en calculant et en v�rifiant les valeurs de CRC16. La fonction ajuste �galement
    l'�tat de la broche de demande de transmission (RTS) pour g�rer le contr�le
    de flux de la r�ception.
 * 
    Param�tres :
    pData : Un pointeur vers la structure de param�tres PWM (S_pwmSettings) o� seront
    stock�es les donn�es de vitesse et d'angle re�ues.
 * 
    Retour :
    La fonction retourne le statut de la communication, indiquant si le message re�u
    �tait valide ou non.
******************************************************************************/

// Valeur de retour 0  = pas de message re�u donc local (data non modifi�)
// Valeur de retour 1  = message re�u donc en remote (data mis � jour)
int GetMessage(S_pwmSettings *pData)
{
    // Traitement de r�ception � introduire ICI
    // Lecture et d�codage fifo r�ception
    // ...
    uint8_t NbCharToRead = 0;
    static uint8_t NbrCycle = 0;
    static uint8_t CommStatus = 0;
    uint32_t ValCRC = 0xFFFF;
    U_manip16 CRC16;
    
    // Obtient le nombre de caract�res � lire depuis le FIFO de r�ception.
    NbCharToRead = GetReadSize(&descrFifoRX);
    
    // Obtient le caract�re de d�but du message depuis le FIFO de r�ception.
    GetCharFromFifo(&descrFifoRX, &RxMess.Start);

    // Obtention du nombre d'octets dans le FIFO et mettre cette valeur dans NbCharToRead 
    // V�rifie si suffisamment de caract�res ont �t� re�us et si le caract�re de d�but est correct.
    if ((NbCharToRead >= MESS_SIZE) && (RxMess.Start == STX_code))
    {
        // Obtient les caract�res de vitesse, d'angle et de CRC depuis le FIFO de r�ception.
        GetCharFromFifo(&descrFifoRX, &RxMess.Speed);
        GetCharFromFifo(&descrFifoRX, &RxMess.Angle);	 
        GetCharFromFifo(&descrFifoRX, &RxMess.MsbCrc);
        GetCharFromFifo(&descrFifoRX, &RxMess.LsbCrc);

        // Calcule la valeur du CRC16 � partir des octets re�us.
        CRC16.shl.lsb = RxMess.LsbCrc;
        CRC16.shl.msb = RxMess.MsbCrc;
        

        // Met � jour la valeur CRC16 en ajoutant les octets du message.
        ValCRC = updateCRC16(ValCRC, RxMess.Start);
        ValCRC = updateCRC16(ValCRC, RxMess.Speed);
        ValCRC = updateCRC16(ValCRC, RxMess.Angle);

        // V�rifie si le CRC16 calcul� correspond au CRC16 re�u.
        if (ValCRC == CRC16.val)
        {   
            // Met � jour les param�tres de l'angle et de la vitesse avec les valeurs re�ues.
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
            // R�initialise le nombre de cycles et le statut de la communication.
            NbrCycle = 0;
            CommStatus = 1;
        }
        
        // Si la CRC16 ne correspond pas et si le nombre de cycles n'a pas atteint 10, incr�mente le nombre de cycles.
        else if (NbrCycle < CYCLE_MAX)
        {
            BSP_LEDToggle(BSP_LED_6);
            NbrCycle++;
        }
    }    
    // Si le nombre de cycles n'a pas atteint 10 et si le message est incomplet, incr�mente le nombre de cycles.
    else
    {
        if (NbrCycle < CYCLE_MAX)
        {
            NbrCycle++;
        }
        // Si le nombre de cycles atteint 10, r�initialise le statut de la communication et le nombre de cycles.
        else
        {
            CommStatus = 0;
            NbrCycle = 0;
        }
    }
    // Gestion controle de flux de la r�ception
    if(GetWriteSpace ( &descrFifoRX) >= (2*MESS_SIZE))
    {
        // autorise �mission par l'autre
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
    R�sum� :
    Cette fonction pr�pare et envoie un message via l'interface de communication,
    en utilisant les informations contenues dans la structure de param�tres PWM.
 * 
    Description :
    La fonction calcule un nouveau CRC16 bas� sur les param�tres de vitesse et d'angle de la structure pData.
    Elle stocke ensuite ce CRC16 avec les donn�es dans une structure TxMess.
    Si l'espace est disponible dans le FIFO de transmission, elle y �crit les donn�es.
    De plus, elle g�re le contr�le de flux en activant une interruption de transmission
    lorsque la broche de demande de transmission (CTS) est d�sactiv�e et qu'il y a de l'espace disponible dans le FIFO.
 *
    Param�tres :
    pData : Un pointeur vers la structure de param�tres PWM (S_pwmSettings)
*/

// Fonction d'envoi des messages, appel cyclique
void SendMessage(S_pwmSettings *pData)
{
    int8_t freeSize;
    uint8_t TransmitCRCHigh = 0;
    uint8_t TransmitCRCLow = 0;
    uint32_t NewCRC = 0xFFFF;
    // Traitement �mission � introduire ICI
    // Formatage message et remplissage fifo �mission
    // Obtient l'espace disponible dans le FIFO de transmission.
    freeSize = GetWriteSpace(&descrFifoTX);

    // V�rifie si suffisamment d'espace est disponible dans le FIFO pour �crire le message complet.
    if(freeSize >= MESS_SIZE)
    {
        // Calcule le nouveau CRC16 � partir des param�tres de vitesse et d'angle.
        NewCRC = updateCRC16(NewCRC, STX_code);
        NewCRC = updateCRC16(NewCRC, pData->SpeedSetting);
        NewCRC = updateCRC16(NewCRC, pData->AngleSetting);

        // S�pare le CRC16 en octets MSB et LSB.
        TransmitCRCHigh = (NewCRC & 0xFF00) >> 8;
        TransmitCRCLow = NewCRC & 0x00FF;

        // Remplit la structure TxMess avec les param�tres et le CRC16.
        TxMess.Start = STX_code;
        TxMess.Speed = pData->SpeedSetting;
        TxMess.Angle = pData->AngleSetting;
        TxMess.MsbCrc = TransmitCRCHigh;
        TxMess.LsbCrc = TransmitCRCLow;

        // �crit chaque �l�ment de la structure TxMess dans le FIFO de transmission.
        PutCharInFifo(&descrFifoTX, TxMess.Start);
        PutCharInFifo(&descrFifoTX, TxMess.Speed);
        PutCharInFifo(&descrFifoTX, TxMess.Angle);
        PutCharInFifo(&descrFifoTX, TxMess.MsbCrc);
        PutCharInFifo(&descrFifoTX, TxMess.LsbCrc);
    }    
    // Gestion du controle de flux
    // si on a un caract�re � envoyer et que CTS = 0
    freeSize = GetWriteSpace(&descrFifoTX);
    if ((RS232_CTS == 0) && (freeSize > 0))
    {        
        // Autorise int �mission    
        PLIB_INT_SourceEnable(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);                
    }
}


// Interruption USART1
// !!!!!!!!
// Attention ne pas oublier de supprimer la r�ponse g�n�r�e dans system_interrupt
// !!!!!!!!
void __ISR(_UART_1_VECTOR, ipl5AUTO) _IntHandlerDrvUsartInstance0(void)
{    
    uint8_t dataAvaliable = 0;
    uint8_t freeSize, TXSize;
    uint8_t byteUsart = 0;
    int8_t c;
    int8_t i_cts = 0;
    BOOL TxBuffFull;
    USART_ERROR UsartStatus;
 
    // Marque d�but interruption avec Led3
    LED3_W = 1;
    // Is this an Error interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_ERROR) && PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_ERROR) )
    {
        /* Clear pending interrupt */
        PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_ERROR);
        // Traitement de l'erreur � la r�ception.
        while (PLIB_USART_ReceiverDataIsAvailable(USART_ID_1))
        {
            PLIB_USART_ReceiverByteReceive(USART_ID_1);
        }
    }
 
    // Is this an RX interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_RECEIVE) && PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_RECEIVE) ) 
    {
                
 
        // Oui Test si erreur parit� ou overrun
        UsartStatus = PLIB_USART_ErrorsGet(USART_ID_1);
 
        if ( (UsartStatus & (USART_ERROR_PARITY | USART_ERROR_FRAMING | USART_ERROR_RECEIVER_OVERRUN)) == 0)
        {
 
            // Traitement RX � faire ICI
            // Lecture des caract�res depuis le buffer HW -> fifo SW
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
 
        
        // Traitement controle de flux reception � faire ICI
        // Gerer sortie RS232_RTS en fonction de place dispo dans fifo reception
        // ...
        freeSize = GetWriteSpace(&descrFifoRX);
        if (freeSize <= 6){
            //controle de flux : demande stop �mission
            RS232_RTS = 1 ;
        }        
    } // end if RX
    
    // Is this an TX interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT) && PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT) )        
    {
 
        // Traitement TX � faire ICI
        // Envoi des caract�res depuis le fifo SW -> buffer HW
        TXSize = GetReadSize (&descrFifoTX);
        i_cts = RS232_CTS;
        // Avant d'�mettre, on v�rifie 3 conditions :
        //  Si CTS = 0 autorisation d'�mettre (entr�e RS232_CTS)
        //  S'il y a un carat�res � �mettre dans le fifo
        //  S'il y a de la place dans le buffer d'�mission (PLIB_USART_TransmitterBufferIsFull)
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
            // (Seulement apr�s TX)

            TXSize = GetReadSize (&descrFifoTX);

            if (TXSize == 0 )
            {
            // pour �viter une interruption inutile
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
        // disable TX interrupt (pour �viter une interrupt. inutile si plus rien � transmettre)
        PLIB_INT_SourceDisable(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);
        
         // Clear the TX interrupt Flag (Seulement apres TX) 
         PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);
 
    // Marque fin interruption avec Led3
    LED3_W = 0;
}

 

 




