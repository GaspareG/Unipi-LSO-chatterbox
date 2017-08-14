=========================================================================  
ISTRUZIONI per il progetto 2016/17
========================================================================  

Il progetto prevede lo sviluppo di un server concorrente che implementa 
una chat: chatterbox. Il nome dato al server e' chatty.
  
------------------------------------------------  
Estrarre il materiale dal KIT  
------------------------------------------------  
Creare una directory temporanea, copiare kit_chatty.tar    
nella directory e spostarsi nella directory appena creata. 

Es.  
$$ mkdir Progetto
$$ tar xvf kit_chatty.tar -C Progetto  
  
questo comando crea nella directory Progetto una directory "chatterbox" con i seguenti file:

chatterbox17.pdf       : specifiche del progetto

client.c	       : client fornito dai docenti che deve essere utilizzato
		       	 per effettuare i tests
			 (NON MODIFICARE)

connections.h
message.h
config.h
ops.h
stats.h                
chatty.c	       : files di riferimento forniti dai docenti.
		         Tale file vanno modificati dallo studente come meglio ritiene.

testconf.sh
testfile.sh
testleaks.sh
teststress.sh
testgroups.sh
Makefile  
		       : programmi di test e Makefile. Nessuno dei file di test va modificato.
                         Il Makefile va modificato nelle parti indicate.
  
DATI/*			: file di dati necessari per i tests
                          (NON MODIFICARE)  
  
README.txt		: questo file  
README.doxygen		: informazioni sul formato doxygen dei commenti alle funzioni 
			  (FACOLTATIVO)
  
------------------  
Come procedere :  
-----------------  
  
1) Leggere attentamente le specifiche. Capire bene il funzionamento del client e di tutti gli 
   script di test forniti dai docenti. Porre particolare attenzione al formato del messaggio
   (vedere file message.h)   
  
2) Implementare le funzioni richieste ed effettuare testing preliminare utilizzando 
   semplici programmi sviluppati allo scopo.
  
3) Testare il software sviluppato con i test forniti dai docenti. Si consiglia di effettuare 
   questa fase solo dopo aver eseguito un buon numero di test preliminari.
  
       bash:~$ make test1  
       bash:~$ make test2  
       bash:~$ make test3
       bash:~$ make test4
       bash:~$ make test5
       bash:~$ make test6
  
  
4) preparare la documentazione: ovvero commentare adeguatamente il/i file che  
   contengono la soluzione  ed inserire un'intestazione contenente il nome  
   dello sviluppatore ed una dichiarazione di originalita'   
  
   /** \file pippo.c  
       \author Giuseppe Garibaldi 456789 
       Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
       originale dell'autore  
     */  
  
  
5) Inserire nel Makefile il nome dei file da consegnare 
   (variabile FILE_DA_CONSEGNARE) il nome del tarball da produrre
   (variabile TARNAME) ed il corso di appartenenza (variabile CORSO)
  
8) eseguire il test 
  
      bash:~$ make consegna  
  
   e seguire le istruzioni del sito del progetto per la consegna.
     
---------------------------------------  
 NOTE IMPORTANTI: LEGGERE ATTENTAMENTE  
---------------------------------------  
  
1) gli eleborati contenenti tar non creati con "make consegna" o
   comunque che non seguono le istruzioni riportate sopra non verranno
   accettati (gli studenti sono invitati a controllare che il tar
   contenga tutti i file richiesti dalla compilazione con il
   comando " tar tvf nomefiletar " prima di inviare il file)
  
2) tutti gli elaborati verranno confrontati fra di loro con tool automatici  
   per stabilire eventuali situazioni di PLAGIO. Se gli elaborati sono   
   simili verranno scartati e gli studenti con file "simili" verranno contattati.
  
3) Chi in sede di orale risulta palesemente non essere l'autore del software  
   consegnato dovra' ricominicare da zero con un altro progetto.
  
4) Tutti i comportamenti scorretti ai punti 2 e 3 verranno segnalati  
   ufficialmente al presidente del corso di laurea.  
  
  
