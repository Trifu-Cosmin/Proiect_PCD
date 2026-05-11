# Protocol comunicare - T17 Semantic Analyzer

## 1. Descriere generala

Protocolul este folosit pentru comunicarea dintre client, server si clientul de administrare.

Aplicatia foloseste socket-uri TCP. Comenzile sunt transmise in format text, iar continutul fisierelor este transmis ca payload dupa header.

Flux general:

```txt
Client normal -> LOGIN -> Server -> UPLOAD/DOWNLOAD_REPORT -> Server -> Client normal
Client admin  -> LOGIN -> Server -> STATS/LOGS/LIST -> Server -> Client admin
```

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

## 3. Drepturi pe roluri

### User normal

Un user normal poate folosi urmatoarele comenzi:

```txt
UPLOAD
DOWNLOAD_REPORT
```

### Admin

Un admin poate folosi urmatoarele comenzi:

```txt
STATS
LOGS
LIST_UPLOADS
LIST_REPORTS
```

---

## 4. Comenzi client normal

### 4.1 UPLOAD

Comanda este folosita pentru trimiterea unui fisier sursa catre server.

Format:

```txt
UPLOAD <filename> <size>
<file_bytes>
```

Exemplu:

```txt
UPLOAD sample.c 450
```

Dupa linia de comanda, clientul trimite exact `size` bytes, reprezentand continutul fisierului.

Serverul:
1. primeste fisierul
2. il salveaza in directorul `uploads/`
3. ruleaza analyzer-ul folosind `fork`, `exec` si `pipe`
4. salveaza raportul in directorul `reports/`
5. trimite raportul rezultat inapoi clientului

---

### 4.2 DOWNLOAD_REPORT

Comanda este folosita pentru descarcarea unui raport generat de server.

Format:

```txt
DOWNLOAD_REPORT <report_name>
```

Exemplu:

```txt
DOWNLOAD_REPORT sample_report.txt
```

Serverul cauta raportul in directorul `reports/` si il trimite catre client.

Raspuns server:

```txt
FILE <filename> <size>
<file_bytes>
```

Clientul salveaza fisierul primit in directorul:

```txt
downloads/
```

Aceasta operatie permite transferul de fisiere si in sensul server -> client.

---

## 5. Comenzi client admin

### 5.1 STATS

Comanda este folosita pentru obtinerea statisticilor serverului.

Format:

```txt
STATS
```

Raspuns posibil:

```txt
Server status: running
Analyzed files: 2
Last file: sample.c
```

---

### 5.2 LOGS

Comanda este folosita pentru afisarea logurilor serverului.

Format:

```txt
LOGS
```

Serverul citeste continutul fisierului:

```txt
logs/server.log
```

si il trimite catre admin client.

---

### 5.3 LIST_UPLOADS

Comanda este folosita pentru listarea fisierelor uploadate pe server.

Format:

```txt
LIST_UPLOADS
```

Serverul listeaza continutul directorului:

```txt
uploads/
```

---

### 5.4 LIST_REPORTS

Comanda este folosita pentru listarea rapoartelor generate.

Format:

```txt
LIST_REPORTS
```

Serverul listeaza continutul directorului:

```txt
reports/
```

---

## 6. Raspunsuri server

### 6.1 RESULT

Serverul foloseste `RESULT` pentru raspunsuri text.

Format:

```txt
RESULT <size>
<payload>
```

Unde:
- `size` este dimensiunea raspunsului in bytes
- `payload` este raportul sau mesajul returnat de server

Exemplu:

```txt
RESULT 120
=== Analysis Result ===
Project: PCD T17 Semantic Analyzer
Diagnostics count: 0
```

---

### 6.2 FILE

Serverul foloseste `FILE` pentru transfer de fisiere catre client.

Format:

```txt
FILE <filename> <size>
<file_bytes>
```

Exemplu:

```txt
FILE sample_report.txt 700
```

Dupa header, serverul trimite exact `size` bytes.

---

## 7. Comenzi implementate

| Comanda | Client | Descriere |
|---|---|---|
| LOGIN | client normal / admin | Autentifica utilizatorul |
| UPLOAD | client normal | Trimite un fisier catre server pentru analiza |
| DOWNLOAD_REPORT | client normal | Descarca un raport generat de server |
| STATS | admin client | Cere statistici de la server |
| LOGS | admin client | Cere logurile serverului |
| LIST_UPLOADS | admin client | Listeaza fisierele uploadate |
| LIST_REPORTS | admin client | Listeaza rapoartele generate |
| RESULT | server | Trimite raspuns text catre client |
| FILE | server | Trimite fisier catre client |

---

## 8. Extensii viitoare

Protocolul poate fi extins cu:

- JOBS
- CLEAR_LOGS
- DELETE_REPORT
- SERVER_STATUS
- SHUTDOWN
- coada de procesare
- sesiuni persistente
- transfer de fisiere in chunks mari