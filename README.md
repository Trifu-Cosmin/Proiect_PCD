# T17 - Analiza Semantica Cod Sursa

Proiect pentru disciplina **Programare Concurenta si Distribuita**.

Aplicatia implementeaza un sistem client-server pentru analiza statica a fisierelor sursa C. Clientii pot trimite fisiere catre server, serverul ruleaza un analyzer bazat pe `libclang`, genereaza rapoarte text si permite administrarea printr-un client separat.

Proiectul include si API-uri minimale REST si SOAP pentru expunerea prin HTTP a informatiilor generate de server: statistici, rapoarte, upload-uri, joburi, useri si loguri.

---

## Functionalitati principale

- server TCP INET pentru clientii normali;
- socket UNIX pentru clientul admin;
- client normal in C;
- client admin in C;
- client remote alternativ in Python;
- autentificare user/admin;
- upload fisiere C catre server;
- analiza fisierelor cu `libclang`;
- generare rapoarte in `reports/`;
- download rapoarte de la server;
- loguri in `logs/server.log`;
- statistici in `logs/stats.txt`;
- job tracking in `logs/jobs.log`;
- monitorizare `reports/` cu `inotify`;
- server concurent cu `select()` si `fork()`;
- REST API minimal;
- SOAP API minimal;
- WSDL pentru SOAP;
- script de test pentru REST;
- script de test pentru SOAP.

---

## Arhitectura

```txt
Client C normal      \
                     -> INET TCP 127.0.0.1:8080 -> Server -> Analyzer -> Report
Client Python        /

Admin Client C       -> UNIX socket /tmp/t17_admin.sock -> Server

Watch Reports        -> inotify -> reports/

REST API             -> HTTP 127.0.0.1:8081 -> logs/, reports/, uploads/, config/

SOAP API             -> HTTP 127.0.0.1:8082 -> logs/, reports/, uploads/, config/
```

Serverul asculta simultan pe doua socket-uri:

- `127.0.0.1:8080` pentru clientii normali;
- `/tmp/t17_admin.sock` pentru clientul admin.

Serverul foloseste `select()` pentru a monitoriza ambele socket-uri si `fork()` pentru a trata fiecare client intr-un proces separat.

REST API-ul ruleaza separat pe `127.0.0.1:8081` si expune raspunsuri JSON.

SOAP API-ul ruleaza separat pe `127.0.0.1:8082`, expune endpoint-ul `/soap` si WSDL la `/wsdl`.

---

## Tehnologii folosite

- C
- Python
- TCP sockets
- UNIX domain sockets
- `select()`
- `fork()`
- `exec()`
- `pipe()`
- `waitpid()`
- `inotify`
- `libclang`
- `libconfig`
- REST API cu `http.server`
- SOAP API minimal
- WSDL
- Makefile
- OpenAPI
- curl

---

## Structura proiectului

```txt
Proiect_PCD/
├── Makefile
├── README.md
├── README.tex
├── config/
│   ├── app.cfg
│   └── users.cfg
├── docs/
│   ├── SRS_SDD.tex
│   ├── protocol.tex
│   └── openapi.yaml
├── python_client/
│   └── client.py
├── rest_api/
│   └── app.py
├── soap_api/
│   └── soap_server.py
├── scripts/
│   ├── test_rest.sh
│   └── test_soap.sh
├── src/
│   ├── analyzer.c
│   ├── main.c
│   ├── server.c
│   ├── client.c
│   ├── admin_client.c
│   └── watch_reports.c
├── tests/
│   ├── sample.c
│   ├── bad.c
│   ├── bad_syntax.c
│   ├── bad_type.c
│   ├── bad_uninitialized.c
│   ├── good_math.c
│   ├── good_nested.c
│   └── good_while.c
├── uploads/
├── reports/
├── logs/
├── downloads/
└── downloads_py/
```

Directoarele `uploads/`, `reports/`, `logs/`, `downloads/` si `downloads_py/` sunt generate la runtime si nu trebuie urcate pe Git.

---

## Dependinte

Pe Ubuntu / WSL:

```bash
sudo apt update
sudo apt install build-essential libclang-dev libconfig-dev python3 curl
```

In Makefile este folosita calea pentru LLVM 18:

```txt
/usr/lib/llvm-18/include
/usr/lib/llvm-18/lib
```

Daca sistemul are alta versiune LLVM, aceste cai trebuie ajustate in `Makefile`.

---

## Compilare

```bash
make clean
make
```

Executabile generate:

```txt
analyzer
main
server
client
admin_client
watch_reports
```

---

## Rulare server

```bash
./server
```

Output asteptat:

```txt
INET server running on 127.0.0.1:8080
UNIX admin socket running on /tmp/t17_admin.sock
Waiting for clients with select()...
```

---

## Client normal C

Upload si analiza fisier:

```bash
./client tests/sample.c
```

Alt exemplu:

```bash
./client tests/bad_type.c
```

Download raport:

```bash
./client download sample_report.txt
```

---

## Client Python

Upload fisier:

```bash
python3 python_client/client.py upload tests/good_math.c
```

Download raport:

```bash
python3 python_client/client.py download good_math_report.txt
```

---

## Admin client

Pornire admin client:

```bash
./admin_client
```

Admin clientul se conecteaza prin socket UNIX:

```txt
/tmp/t17_admin.sock
```

Meniu admin:

```txt
1. Server stats
2. Show server logs
3. List uploaded files
4. List reports
5. Server status
6. Clear server logs
7. Delete report
8. List users
9. Show analysis jobs
10. Quit
```

---

## Monitorizare rapoarte cu inotify

```bash
./watch_reports
```

Apoi, intr-un alt terminal:

```bash
./client tests/sample.c
```

Output posibil:

```txt
Watching directory: reports
Waiting for report changes...
[2026-06-12 12:26:09] report created: sample_report.txt
[2026-06-12 12:26:09] report modified: sample_report.txt
[2026-06-12 12:26:09] report written: sample_report.txt
```

---

## REST API minimal

Pornire:

```bash
python3 rest_api/app.py
```

Adresa:

```txt
http://127.0.0.1:8081
```

Endpoint-uri:

```txt
GET /
GET /health
GET /stats
GET /reports
GET /reports/<name>
GET /uploads
GET /jobs
GET /users
GET /logs
GET /ui
```

Interfata web simpla:

```txt
http://127.0.0.1:8081/ui
```

Test:

```bash
./scripts/test_rest.sh
```

---

## SOAP API minimal

Pornire:

```bash
python3 soap_api/soap_server.py
```

Endpoint SOAP:

```txt
http://127.0.0.1:8082/soap
```

WSDL:

```txt
http://127.0.0.1:8082/wsdl
```

Operatii SOAP:

```txt
Health
Stats
Reports
Uploads
Jobs
Users
Logs
```

Test:

```bash
./scripts/test_soap.sh
curl http://127.0.0.1:8082/wsdl
```

---

## Analyzer

Analyzer-ul foloseste `libclang` pentru a analiza fisiere C.

Exemplu rulare directa:

```bash
./analyzer -f tests/sample.c -v
```

Analyzer-ul extrage:

- numar functii;
- numar variabile;
- numar instructiuni `if`;
- numar instructiuni `for`;
- numar instructiuni `while`;
- diagnostice `libclang`;
- warning-uri si erori.

Serverul ruleaza analyzer-ul folosind `fork()`, `exec()` si `pipe()`.

---

## Autentificare

Utilizatorii sunt definiti in:

```txt
config/users.cfg
```

Format:

```txt
username:password:role
```

Exemplu:

```txt
user1:pass1:user
admin:admin123:admin
```

Login-ul este trimis automat de clienti la inceputul fiecarei conexiuni.

---

## Job tracking

La fiecare upload si analiza, serverul scrie statusul jobului in:

```txt
logs/jobs.log
```

Exemplu:

```txt
[2026-06-12 12:17:08] sample.c QUEUED
[2026-06-12 12:17:08] sample.c RUNNING
[2026-06-12 12:17:09] sample.c DONE
```

Joburile pot fi vazute:

```txt
admin_client -> Show analysis jobs
REST API -> GET /jobs
SOAP API -> Jobs
```

---

## Testare rapida

Terminal 1:

```bash
./server
```

Terminal 2:

```bash
./watch_reports
```

Terminal 3:

```bash
python3 rest_api/app.py
```

Terminal 4:

```bash
python3 soap_api/soap_server.py
```

Terminal 5:

```bash
./client tests/sample.c
./client tests/bad_type.c
./client download sample_report.txt
```

Terminal 6:

```bash
python3 python_client/client.py upload tests/good_math.c
python3 python_client/client.py download good_math_report.txt
```

Terminal 7:

```bash
./admin_client
```

Terminal 8:

```bash
./scripts/test_rest.sh
./scripts/test_soap.sh
```

---

## Documentatie

Documentatia oficiala este in LaTeX:

```txt
docs/SRS_SDD.tex
docs/protocol.tex
README.tex
```

Specificatia REST este in:

```txt
docs/openapi.yaml
```

README-ul principal pentru GitHub ramane:

```txt
README.md
```

---

## Functionalitati bifate pentru proiect

- socket INET TCP;
- socket UNIX / local;
- comunicare client-server;
- client normal C;
- client admin C;
- client in alta limba, Python;
- transfer fisiere client -> server;
- transfer fisiere server -> client;
- autentificare;
- roluri user/admin;
- server concurent cu `fork()`;
- monitorizare socket-uri cu `select()`;
- executie analyzer cu `exec()`;
- comunicare cu analyzer prin `pipe()`;
- job tracking;
- monitorizare cu `inotify`;
- REST API minimal;
- OpenAPI;
- SOAP API minimal;
- WSDL;
- script de test REST;
- script de test SOAP;
- documentatie LaTeX.

---

## Concluzie

Proiectul implementeaza o aplicatie distribuita locala pentru analiza semantica a codului sursa C. Sistemul foloseste concepte importante din programarea concurenta si distribuita: socket-uri INET, socket-uri UNIX, procese, pipe-uri, `select()`, autentificare, transfer de fisiere, monitorizare cu `inotify`, REST API si SOAP API.