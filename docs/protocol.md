# Protocol comunicare - T17 Semantic Analyzer

## 1. Descriere generala

Protocolul este folosit pentru comunicarea dintre client, server si clientul de administrare.

Aplicatia foloseste socket-uri TCP. Comenzile sunt transmise in format text, iar continutul fisierelor este transmis ca payload dupa header.

Flux general:

```txt
Client normal -> Server -> Analyzer -> Server -> Client normal
Client admin  -> Server -> Raspuns statistici
```

---

## 2. Comenzi client normal

### 2.1 UPLOAD

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
4. trimite raportul rezultat inapoi clientului

---
### 2.2 DOWNLOAD_REPORT

Comanda este folosita pentru descarcarea unui raport generat de server.

Format:

```txt```
DOWNLOAD_REPORT <report_name>

Exemplu:
DOWNLOAD_REPORT sample_report.txt

Serverul cauta raportul in directorul reports/ si il trimite catre client.

Raspuns server:
FILE <filename> <size>
<file_bytes>
Clientul salveaza fisierul primit in directorul downloads/.
Aceasta operatie permite transferul de fisiere si in sensul server -> client.

## 3. Comenzi client admin

### 3.1 STATS

Comanda este folosita pentru obtinerea statisticilor serverului.

Format:

```txt
STATS
```

Raspuns posibil:

```txt
Server status: running
Analyzed files: 2
Last file: bad.c
```

---

## 4. Raspunsuri server

Serverul raspunde folosind formatul:

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

## 5. Comenzi implementate

| Comanda | Client | Descriere |
|---|---|---|
| UPLOAD | client normal | Trimite un fisier catre server pentru analiza |
| STATS | admin client | Cere statistici de la server |
| RESULT | server | Trimite raspunsul catre client |

---

## 6. Extensii viitoare

Protocolul poate fi extins cu:

- LOGIN username password
- LOGS
- JOBS
- LIST_REPORTS
- DOWNLOAD_REPORT
- QUIT
- ERROR <message>

Aceste extensii sunt utile pentru Milestone 2, unde trebuie dezvoltate mai multe operatii pentru clientul normal si clientul admin.