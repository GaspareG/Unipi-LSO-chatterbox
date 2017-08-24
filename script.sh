#
# chatterbox Progetto del corso di LSO 2017
#
# Dipartimento di Informatica Università di Pisa
# Docenti: Prencipe, Torquati
#
# Autore: Gaspare Ferraro CORSO B - Matricola 520549
# Tale sorgente è, in ogni sua parte, opera originale di Gaspare Ferraro
#
# @file script.sh
# @brief script con le funzionalità richieste
#

#!/bin/bash

# Se ho meno di due parametri 
# Stampo l'uso dello script
if [ $# -lt 2 ]; 
then
    echo "Usage: $0 file.conf time" 1>&2
    exit 1
fi

# Se --help è passato come argomento
# Stampo l'uso dello script
for Par in "$@"
do  
    if [ "$Par" == "--help" ];
    then
        echo "Usage: $0 file.conf time" 1>&2
        exit 1
    fi
done

# Controllo se il primo parametro 
# sia effettivamente un file esistente
if ! [ -f $1 ];
then
    echo "Error: File $1 not exists!" 1>&2
    exit 2
fi

# Estraggo i due parametri

# Prendo il contenuto del file
DirName="$(cat $1)"

#Tolgo il prefisso
DirName=${DirName##*"DirName"}
DirName=${DirName#*"="}
DirName=${DirName#*'\s'}

#Tolgo il suffisso
DirName=${DirName%%'#'*}
DirName=${DirName//[[:space:]]/}

#Tolto l'estensione
Time=$2

# Se t == 0 
if [ $Time -eq 0 ];
then

    # Stampo tutti i file contenuti nella directory DirName
    echo "Files dentro a [$DirName]:" 1>&2

    # Lista di file nella cartella
    Files=$DirName/*

    # Scorro l'array
    for F in $Files
    do
        # Se è un file vero lo stampo
        if [ -f $F ];
        then  
            FileFame=${F##*/}
            echo "- $FileFame" 1>&2
        fi
    done

# Se t > 0
elif [ $Time -gt 0 ];
then

    #echo "Time > 0" 1>&2   
    echo "Files dentro a $DirName:" 1>&2

    # Lista di file nella cartella
    Files=$DirName/*

    DateNow=$(date +%s)
    # Scorro l'array
    for F in $Files
    do
        # Ottengo l'età in minuti del file
        DateFile=$(stat -c %Y $F)
        MinutesLife=$(( ($DateNow - $DateFile)/60 ))
        # Se supera i minuti richiesti
        if [ $MinutesLife -gt $Time ];
        then
            echo "$F - $MinutesLife minuti" 1>&2
            # Cancello il file 
            # Per sicurezza uso l'opzione -i per la conferma
            rm -i -R $F
        fi
    done

# Altrimenti errore
else

    echo "Errore: il tempo deve essere maggiore di 0!" 1>&2
    exit 3

fi

exit 0
