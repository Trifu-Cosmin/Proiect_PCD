# Protocol comunicare - T17 Semantic Analyzer

## Descriere

Acest fisier descrie protocolul simplu folosit intre client, server si clientul de administrare.

Protocolul este text-based pentru comenzi, iar transferul fisierului se face ca payload binar dupa header.

---

## Comenzi client normal

### UPLOAD

Format:

```txt
UPLOAD <filename> <size>
<file_bytes>

Exemplu:
UPLOAD sample.c 350
Dupa aceasta linie, clientul trimite exact 350 bytes reprezentand continutul fisierului.

Serverul:

primeste fisierul
il salveaza in uploads/
ruleaza analyzer-ul
trimite rezultatul inapoi
Comenzi client admin

STATS

Format:STATS

Serverul raspunde cu statistici simple:

Server status: running
Analyzed files: 2
Last file: sample.c

Raspuns server

Toate raspunsurile importante sunt trimise in format:
RESULT <size>
<payload>

Unde:

size reprezinta dimensiunea raspunsului in bytes
payload contine raportul sau mesajul de eroare

Observatii

Protocolul este simplu pentru aceasta etapa si poate fi extins ulterior cu:

autentificare
sesiuni
comenzi admin suplimentare
upload/download bidirectional
coduri de status