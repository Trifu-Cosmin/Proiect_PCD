# Protocol comunicare - T17 Semantic Analyzer

## 1. Descriere generala

Protocolul este folosit pentru comunicarea dintre clientul normal, clientul admin, clientul Python si serverul TCP.

Aplicatia foloseste socket-uri TCP. Comenzile sunt transmise in format text, iar continutul fisierelor este transmis ca payload dupa header.

Flux general:

```txt
Client normal C -> LOGIN -> Server -> UPLOAD / DOWNLOAD_REPORT -> Server -> Client normal C
Client Python   -> LOGIN -> Server -> UPLOAD / DOWNLOAD_REPORT -> Server -> Client Python
Client admin C  -> LOGIN -> Server -> STATS / LOGS / LIST_UPLOADS / LIST_REPORTS -> Server -> Client admin C
```

Serverul trateaza conexiunile concurent folosind `fork()`. Pentru fiecare client acceptat, serverul porneste un proces copil care se ocupa de cererea respectiva.

---

## 2. Autentificare

Prima comanda trimisa catre server trebuie sa fie `LOGIN`.

Format:

```txt
LOGIN <username> <password>
```

Exemplu pentru client normal:

```txt
LOGIN user1 pass1
```

Exemplu pentru client admin:

```txt
LOGIN admin admin123
```

Serverul verifica datele in fisierul:

```txt
config/users.cfg
```

Formatul fisierului este:

```txt
username:password:role
```

Exemplu:

```txt
user1:pass1:user
admin:admin123:admin
```

Daca autentificarea reuseste, serverul raspunde cu:

```txt
RESULT <size>
OK role=user
```

sau:

```txt
RESULT <size>
OK role=admin
```

Daca autentificarea esueaza, serverul raspunde cu:

```txt
RESULT <size>
ERROR: invalid credentials
```

---

## 3. Roluri si permisiuni

### 3.1 User normal

Un user normal poate folosi urmatoarele comenzi:

```txt
UPLOAD
DOWNLOAD_REPORT
```

Acest rol este folosit de:
- clientul normal C
- clientul Python

---

### 3.2 Admin

Un admin poate folosi urmatoarele comenzi:

```txt
STATS
LOGS
LIST_UPLOADS
LIST_REPORTS
```

Acest rol este folosit de:
- clientul admin C

---

## 4. Comenzi client normal

## 4.1 UPLOAD

Comanda `UPLOAD` este folosita pentru trimiterea unui fisier sursa catre server.

Format:

```txt
UPLOAD <filename> <size>
<file_bytes>
```

Exemplu:

```txt
UPLOAD sample.c 582
```

Dupa linia de comanda, clientul trimite exact `size` bytes, reprezentand continutul fisierului.

Serverul:
1. primeste fisierul
2. il salveaza in directorul `uploads/`
3. ruleaza analyzer-ul folosind `fork`, `exec` si `pipe`
4. salveaza raportul in directorul `reports/`
5. trimite raportul rezultat inapoi clientului

Raspuns server:

```txt
RESULT <size>
<payload>
```

Exemplu raspuns:

```txt
RESULT 250
=== Analysis Result ===
Project: PCD T17 Semantic Analyzer
Source file: uploads/sample.c
Diagnostics count: 0
Functions count: 3
Variables count: 8
If statements count: 1
For loops count: 1
While loops count: 0
```

---

## 4.2 DOWNLOAD_REPORT

Comanda `DOWNLOAD_REPORT` este folosita pentru descarcarea unui raport generat de server.

Format:

```txt
DOWNLOAD_REPORT <report_name>
```

Exemplu:

```txt
DOWNLOAD_REPORT sample_report.txt
```

Serverul cauta raportul in directorul:

```txt
reports/
```

Daca raportul exista, serverul il trimite catre client folosind raspuns de tip `FILE`.

Raspuns server:

```txt
FILE <filename> <size>
<file_bytes>
```

Exemplu:

```txt
FILE sample_report.txt 700
```

Clientul C salveaza fisierul primit in:

```txt
downloads/
```

Clientul Python salveaza fisierul primit in:

```txt
downloads_py/
```

Aceasta comanda permite transferul fisierelor si in sensul server -> client.

---

## 5. Comenzi client admin

## 5.1 STATS

Comanda `STATS` este folosita pentru obtinerea statisticilor serverului.

Format:

```txt
STATS
```

Raspuns posibil:

```txt
RESULT <size>
Server status: running
Analyzed files: 2
Last file: sample.c
```

Informatiile sunt citite din fisierul persistent:

```txt
logs/stats.txt
```

---

## 5.2 LOGS

Comanda `LOGS` este folosita pentru afisarea logurilor serverului.

Format:

```txt
LOGS
```

Serverul citeste continutul fisierului:

```txt
logs/server.log
```

si il trimite catre admin client.

Raspuns posibil:

```txt
RESULT <size>
[2026-05-13 14:54:29] INFO Concurrent server started on 127.0.0.1:8080
[2026-05-13 14:54:39] INFO Client connected in child process
[2026-05-13 14:54:39] INFO Authentication successful for user: user1, role: user
```

---

## 5.3 LIST_UPLOADS

Comanda `LIST_UPLOADS` este folosita pentru listarea fisierelor uploadate pe server.

Format:

```txt
LIST_UPLOADS
```

Serverul listeaza continutul directorului:

```txt
uploads/
```

Raspuns posibil:

```txt
RESULT <size>
Uploaded files:
- sample.c
- bad.c
```

---

## 5.4 LIST_REPORTS

Comanda `LIST_REPORTS` este folosita pentru listarea rapoartelor generate.

Format:

```txt
LIST_REPORTS
```

Serverul listeaza continutul directorului:

```txt
reports/
```

Raspuns posibil:

```txt
RESULT <size>
Generated reports:
- sample_report.txt
- bad_report.txt
```

---

## 6. Raspunsuri server

## 6.1 RESULT

Serverul foloseste `RESULT` pentru raspunsuri text.

Format:

```txt
RESULT <size>
<payload>
```

Unde:
- `size` reprezinta dimensiunea raspunsului in bytes
- `payload` contine raportul, mesajul de eroare sau informatia ceruta

Exemple:

```txt
RESULT 13
OK role=user
```

```txt
RESULT 27
ERROR: invalid credentials
```

```txt
RESULT 120
Server status: running
Analyzed files: 2
Last file: sample.c
```

---

## 6.2 FILE

Serverul foloseste `FILE` pentru transfer de fisiere catre client.

Format:

```txt
FILE <filename> <size>
<file_bytes>
```

Unde:
- `filename` este numele fisierului trimis
- `size` este dimensiunea fisierului in bytes
- `file_bytes` reprezinta continutul fisierului

Exemplu:

```txt
FILE sample_report.txt 700
```

Dupa header, serverul trimite exact `size` bytes.

---

## 7. Comenzi implementate

| Comanda | Client | Descriere |
|---|---|---|
| LOGIN | client normal / admin / Python | Autentifica utilizatorul |
| UPLOAD | client normal / Python | Trimite un fisier catre server pentru analiza |
| DOWNLOAD_REPORT | client normal / Python | Descarca un raport generat de server |
| STATS | admin client | Cere statistici de la server |
| LOGS | admin client | Cere logurile serverului |
| LIST_UPLOADS | admin client | Listeaza fisierele uploadate |
| LIST_REPORTS | admin client | Listeaza rapoartele generate |
| RESULT | server | Trimite raspuns text catre client |
| FILE | server | Trimite fisier catre client |

---

## 8. Directoare folosite de protocol

```txt
uploads/
```

Contine fisierele sursa primite de la clienti.

```txt
reports/
```

Contine rapoartele generate de analyzer.

```txt
logs/
```

Contine logurile serverului si statisticile persistente.

```txt
downloads/
```

Contine rapoartele descarcate de clientul C.

```txt
downloads_py/
```

Contine rapoartele descarcate de clientul Python.

---

## 9. Exemple de fluxuri

## 9.1 Analiza fisier cu client C

```txt
Client -> Server:
LOGIN user1 pass1

Server -> Client:
RESULT 13
OK role=user

Client -> Server:
UPLOAD sample.c 582
<file_bytes>

Server -> Client:
RESULT <size>
<analysis_report>
```

---

## 9.2 Descarcare raport cu client C

```txt
Client -> Server:
LOGIN user1 pass1

Server -> Client:
RESULT 13
OK role=user

Client -> Server:
DOWNLOAD_REPORT sample_report.txt

Server -> Client:
FILE sample_report.txt 700
<file_bytes>
```

---

## 9.3 Cerere statistici admin

```txt
Admin -> Server:
LOGIN admin admin123

Server -> Admin:
RESULT 14
OK role=admin

Admin -> Server:
STATS

Server -> Admin:
RESULT <size>
Server status: running
Analyzed files: 2
Last file: sample.c
```

---

## 10. Erori posibile

Daca un client trimite o comanda fara LOGIN:

```txt
RESULT <size>
ERROR: login required
```

Daca autentificarea esueaza:

```txt
RESULT <size>
ERROR: invalid credentials
```

Daca userul nu are permisiune:

```txt
RESULT <size>
ERROR: permission denied
```

Daca un user normal incearca o comanda admin:

```txt
RESULT <size>
ERROR: admin permission required
```

Daca raportul cerut nu exista:

```txt
RESULT <size>
ERROR: report file not found
```

Daca o comanda este necunoscuta:

```txt
RESULT <size>
ERROR: unknown command
```

---

## 11. Extensii viitoare

Protocolul poate fi extins cu:

- JOBS
- CLEAR_LOGS
- DELETE_REPORT
- SERVER_STATUS
- SHUTDOWN
- job queue real
- sesiuni persistente
- transfer de fisiere mari in chunks
- coduri de status mai clare