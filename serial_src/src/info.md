Questo codice sorgente serve per ricevere via html un comando e impartirlo via rs232 al telescopio MEADE lx200gps-acf AUTOSTAR II da 12'
Sono implementati i seguenti comandi, copiati direttamente dalla libreria di comandi di stellarium.

comando:    <0x06>
    - returna: A, L, o P in base alla modalità di allineamento

comando:    :CM#
    - comando di sincronizzazione che devo ancora capirne il senso
    - returna: M31 EX GAL MAG 3.5 SZ178.0'
        - nota: sul telescopio del cassini questa stringa è statica

comando:    :GD#
    - richiesta al telescopio della declinazione
    - returna: sDD*MM'SS# oppure sDD*MM# in base alla precisione impostata
        - nota: se la precisione è bassa bisogna impostarla alta!!

comando:    :GR# 
    - richiesta al telescopio dell'angolo alla quale è posizionato
    - returna: HH:MM:SS# o HH:MM.T# in base alla precisione impostata
        - nota: se la precisione è bassa bisogna reimpostarla alta!

comando:    :MS#    NO DEPRECATO
    - avvia il telescopio verso il punto obbiettivo (NON FUNZIONA CON TELESC CASSINI)
    - returna: 0 se successo, 1<stringa># se sotto l'orizzonte, 2<stringa># se troppo sopra
    - esempio: 1Below Horizon#

comando:    :MA#
    - avvia il telescopio verso il punto obbiettivo
    - returna 0 se ok 1 se problemi

comando:    :Q#
    - freno di emergenza
    - returna: niente

comando:    :SdsDD*MM:SS#
    - imposta la declinazione obbiettivo
    - returna: 1 se valido, 0 se rifiutato

comando:    :SrHH:MM:SS#
    - imposta la RA obbiettivo
    - returna: 1 se valido, 0 se rifiutato

comando:    :U#
    - cambia la precisione da alta a bassa e v.v.
    - returna: niente

comando:    :I#
    - stoppa tutto e resetta il telescopio alla sua posizione di parcheggio
    - TODO: testare

comando:    :Gh# (e :Go#?)
    - returna sDD* elevazione minima per cui non da "below horizon"

potrebbero esserci altri comandi utili non usufruiti da stellarium per impostare/leggere l'ora

        
====================== DUMP TEST DI COMUNICAZIONE =====================
---- Sent utf8 encoded message: ":GR#" ----
02:05:25#
---- Sent utf8 encoded message: ":U#" ----
---- Sent utf8 encoded message: ":GR#" ----
02:06.5#
---- Sent utf8 encoded message: ":U#" ----
---- Sent utf8 encoded message: ":GR#" ----
02:06:39#

---- Sent utf8 encoded message: ":GD#" ----
-06�35:59#

//get local time
---- Sent utf8 encoded message: ":GL#" ----
20:12:38#

//NOTA: A VOLTE MISSA
---- Sent utf8 encoded message: ":GT#" ----
+60.0#
//aggiorna data
---- Sent utf8 encoded message: ":SC06/23/26#" ----
1Updating planetary data        #                               #
---- Sent utf8 encoded message: ":GC#" ----
06/23/26#
//get azimuth
---- Sent utf8 encoded message: ":GZ#" ----
105�50:26#

---- Sent utf8 encoded message: ":Sr10:10:00#" ----
1
---- Sent utf8 encoded message: ":MS#" ----
1Below Horizon#
//in questo caso Sr ha returnato 1 come valido, tuttavia al comando 
//:MS# ha throwato errore. 

---- Sent utf8 encoded message: ":Gh#" ----
90�#
---- Sent utf8 encoded message: ":Go#" ----
-33�#
---- Sent utf8 encoded message: ":CM#" ----
   M31    EX GAL MAG 3.5 SZ178.0'#
---- Sent hex encoded message: "06" ----
A

SENT: :GD#
REPLY: 2b 30 35 df 33 36 3a 31 39 23

---- Sent utf8 encoded message: ":Sr00:00:00#" ----
1
---- Sent utf8 encoded message: ":MS#" ----
0
---- Sent utf8 encoded message: ":GR#" ----
16:32:35#
---- Sent utf8 encoded message: ":GR#" ----
07:23:34#
---- Sent utf8 encoded message: ":GR#" ----
01:08:35#
---- Sent utf8 encoded message: ":Sr00:00:00#" ----
1
---- Sent utf8 encoded message: ":MA#" ----
0
---- Sent utf8 encoded message: ":GR#" ----
12:01:26#
---- Sent utf8 encoded message: ":Sr00:00:00#" ----
1
---- Sent utf8 encoded message: ":GR#" ----
12:01:35#
---- Sent utf8 encoded message: ":MA#" ----
0
---- Sent utf8 encoded message: ":Sd+10*00:00#" ----
1
---- Sent utf8 encoded message: ":MA#" ----
0
---- Sent utf8 encoded message: ":GD#" ----
+09�54:35#
---- Sent utf8 encoded message: ":Sr10:00:00#" ----
1
---- Sent utf8 encoded message: ":Sd+05*00:00#" ----
1
---- Sent utf8 encoded message: ":MA#" ----
0
---- Sent utf8 encoded message: ":Sd+30*00:00#" ----
1
---- Sent utf8 encoded message: ":Sr05:00:00#" ----
1
---- Sent utf8 encoded message: ":MS#" ----
0
---- Sent utf8 encoded message: ":MA#" ----
0
---- Sent utf8 encoded message: ":Sd-40*00:00#" ----
1
---- Sent utf8 encoded message: ":MS#" ----
1Below Horizon#
---- Sent utf8 encoded message: ":Sd+95*00:00#" ----
1
---- Sent utf8 encoded message: ":Sd+99*99:00#" ----
1
---- Sent utf8 encoded message: ":MS#" ----
0
---- Sent utf8 encoded message: ":Sd+30*00:00#" ----
1
---- Sent utf8 encoded message: ":Sr10:00:00#" ----
1
---- Sent utf8 encoded message: ":MA#" ----