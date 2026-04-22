#define _DEFAULT_SOURCE

/*
  Descriere:
  Acest fisier contine programul principal al proiectului pentru tema T17.

  Aplicatia:
  - citeste configuratia din fisierul config/app.cfg folosind libconfig
  - primeste fisierul sursa fie din linia de comanda (-f), fie din variabila de mediu SOURCE_FILE
  - foloseste libclang pentru a parsa fisierul sursa
  - extrage informatii despre structura codului (AST):
        * numar functii
        * numar variabile
        * numar instructiuni if
        * numar bucle for
        * numar bucle while
  - afiseaza diagnosticele (erori/warnings) daca optiunea este activa

  Observatie:
  Codul reprezinta baza pentru extindere ulterioara (client-server / REST API).
*/

#include <clang-c/Index.h> // Pentru analiza codului (libclang)
#include <libconfig.h>     // Pentru citirea configuratiei
#include <stdio.h>         // Pentru printf, fprintf
#include <stdlib.h>        // Pentru getenv
#include <string.h>        // Pentru operatii pe stringuri
#include <unistd.h>        // Pentru getopt
#include <getopt.h>        // Pentru getopt si optarg

/*
  Structura folosita pentru a retine statistici despre codul analizat.
*/
typedef struct
{
    unsigned functions;
    unsigned variables;
    unsigned if_statements;
    unsigned for_loops;
    unsigned while_loops;
} AnalysisStats;

/*
  Functie auxiliara:
  Afiseaza modul de utilizare al programului.
*/
static void print_usage(const char *program_name)
{
    printf("Utilizare: %s [-f fisier] [-v] [-h]\n", program_name);
    printf("  -f <fisier>  specifica fisierul sursa analizat\n");
    printf("  -v           afiseaza diagnosticele\n");
    printf("  -h           afiseaza ajutor\n");
}

/*
  Functie callback pentru parcurgerea AST-ului generat de libclang.

  Pentru fiecare nod din arbore:
  - verificam tipul nodului (functie, variabila, if, etc.)
  - incrementam contorul corespunzator
*/
static enum CXChildVisitResult ast_visitor(CXCursor cursor, CXCursor parent, CXClientData client_data)
{
    (void)parent;

    AnalysisStats *stats = (AnalysisStats *)client_data;
    enum CXCursorKind kind = clang_getCursorKind(cursor);

    if (kind == CXCursor_FunctionDecl && clang_isCursorDefinition(cursor))
    {
        stats->functions++;
    }
    else if (kind == CXCursor_VarDecl)
    {
        stats->variables++;
    }
    else if (kind == CXCursor_IfStmt)
    {
        stats->if_statements++;
    }
    else if (kind == CXCursor_ForStmt)
    {
        stats->for_loops++;
    }
    else if (kind == CXCursor_WhileStmt)
    {
        stats->while_loops++;
    }

    return CXChildVisit_Recurse;
}

int main(int argc, char *argv[])
{
    config_t cfg; // structura configuratie
    config_setting_t *clang_args_setting = NULL;

    const char *project_name = NULL;
    int show_diagnostics_from_config = 0;

    /*
      Variabile pentru argumente CLI
    */
    const char *source_file_cli = NULL;
    int force_verbose = 0;

    /*
      Parsare argumente din linia de comanda
    */
    int opt;
    while ((opt = getopt(argc, argv, "f:vh")) != -1)
    {
        switch (opt)
        {
        case 'f':
            source_file_cli = optarg;
            break;
        case 'v':
            force_verbose = 1;
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            return 1;
        }
    }

    /*
      Initializare libconfig
    */
    config_init(&cfg);

    /*
      Citire fisier configurare
    */
    if (!config_read_file(&cfg, "config/app.cfg"))
    {
        fprintf(stderr, "Eroare la citirea fisierului config\n");
        config_destroy(&cfg);
        return 1;
    }

    /*
      Citire valori din config
    */
    config_lookup_string(&cfg, "project_name", &project_name);
    config_lookup_bool(&cfg, "show_diagnostics", &show_diagnostics_from_config);

    /*
      Citire lista argumente pentru clang
    */
    clang_args_setting = config_lookup(&cfg, "clang_args");

    /*
      Determinare fisier sursa:
      prioritate: CLI -> variabila de mediu
    */
    const char *source_file = source_file_cli;

    if (!source_file)
    {
        source_file = getenv("SOURCE_FILE");
    }

    if (!source_file)
    {
        fprintf(stderr, "Nu a fost specificat fisierul sursa\n");
        config_destroy(&cfg);
        return 1;
    }

    /*
      Stabilire mod afisare diagnostice
    */
    int show_diagnostics = force_verbose ? 1 : show_diagnostics_from_config;

    /*
      Construire lista argumente pentru clang
    */
    int arg_count = config_setting_length(clang_args_setting);
    const char *clang_args[16];

    for (int i = 0; i < arg_count; i++)
    {
        clang_args[i] = config_setting_get_string_elem(clang_args_setting, i);
    }

    /*
      Creare index libclang
    */
    CXIndex index = clang_createIndex(0, 0);

    /*
      Parsare fisier sursa
      (IMPORTANT: folosim DetailedPreprocessingRecord pentru diagnostice mai bune)
    */
    CXTranslationUnit tu = clang_parseTranslationUnit(
        index,
        source_file,
        clang_args,
        arg_count,
        NULL,
        0,
        CXTranslationUnit_DetailedPreprocessingRecord);

    if (!tu)
    {
        fprintf(stderr, "Eroare la parsarea fisierului\n");
        return 1;
    }

    /*
      Analiza AST
    */
    AnalysisStats stats = {0};
    CXCursor root = clang_getTranslationUnitCursor(tu);
    clang_visitChildren(root, ast_visitor, &stats);

    /*
      Numar diagnostice
    */
    unsigned diag_count = clang_getNumDiagnostics(tu);

    /*
      Afisare rezultate
    */
    printf("=== Analysis Result ===\n");
    printf("Project: %s\n", project_name);
    printf("Source file: %s\n", source_file);
    printf("Diagnostics count: %u\n", diag_count);

    printf("Functions count: %u\n", stats.functions);
    printf("Variables count: %u\n", stats.variables);
    printf("If statements count: %u\n", stats.if_statements);
    printf("For loops count: %u\n", stats.for_loops);
    printf("While loops count: %u\n", stats.while_loops);

    /*
      Afisare diagnostice (daca sunt activate)
    */
    if (show_diagnostics)
{
    printf("\n=== Diagnostics ===\n");

    if (diag_count == 0)
    {
        printf("Nu exista diagnostice.\n");
    }
    else
    {
        for (unsigned i = 0; i < diag_count; i++)
        {
            CXDiagnostic d = clang_getDiagnostic(tu, i);
            CXString text = clang_formatDiagnostic(
                d,
                clang_defaultDiagnosticDisplayOptions());

            printf("%s\n", clang_getCString(text));

            clang_disposeString(text);
            clang_disposeDiagnostic(d);
        }
    }
}

    printf("\nParse completed.\n");

    /*
      Eliberare resurse
    */
    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);
    config_destroy(&cfg);

    return 0;
}