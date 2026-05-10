#define _DEFAULT_SOURCE

/*
  Descriere:
  Acest fisier contine modulul principal de analiza pentru tema T17.

  Programul:
  - citeste configuratia din config/app.cfg folosind libconfig
  - primeste fisierul sursa prin argument CLI (-f) sau prin variabila SOURCE_FILE
  - foloseste libclang pentru parsarea fisierului sursa
  - parcurge AST-ul pentru a numara functii, variabile, if, for si while
  - afiseaza diagnosticele generate de libclang

  Acest executabil este folosit direct pentru testare locala, dar poate fi
  folosit si de server prin fork/exec.
*/

#include <clang-c/Index.h> // Pentru libclang
#include <getopt.h>        // Pentru getopt si optarg
#include <libconfig.h>     // Pentru libconfig
#include <stdio.h>         // Pentru printf, fprintf
#include <stdlib.h>        // Pentru getenv
#include <unistd.h>        // Pentru getopt

#define MAX_CLANG_ARGS 16

typedef struct
{
    unsigned functions;
    unsigned variables;
    unsigned if_statements;
    unsigned for_loops;
    unsigned while_loops;
} AnalysisStats;

/*
  Afiseaza modul de utilizare pentru program.
*/
static void print_usage(const char *program_name)
{
    printf("Utilizare: %s [-f fisier] [-v] [-h]\n", program_name);
    printf("  -f <fisier>  specifica fisierul sursa analizat\n");
    printf("  -v           afiseaza diagnosticele explicit\n");
    printf("  -h           afiseaza ajutor\n");
}

/*
  Callback folosit de libclang pentru parcurgerea AST-ului.

  Pentru fiecare nod gasit in arbore, verificam tipul nodului si actualizam
  statisticile corespunzatoare.
*/
static enum CXChildVisitResult ast_visitor(CXCursor cursor, CXCursor parent, CXClientData client_data)
{
    (void)parent;

    AnalysisStats *stats = (AnalysisStats *)client_data;
    enum CXCursorKind kind = clang_getCursorKind(cursor);

    if (kind == CXCursor_FunctionDecl && clang_isCursorDefinition(cursor) != 0)
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
    config_t cfg;
    config_setting_t *clang_args_setting = NULL;

    const char *project_name = NULL;
    const char *source_file_cli = NULL;
    int show_diagnostics_from_config = 0;
    int force_verbose = 0;

    /*
      Parsare argumente din linia de comanda.
      -f specifica fisierul sursa
      -v forteaza afisarea diagnosticelor
      -h afiseaza ajutorul
    */
    int option = 0;
    while ((option = getopt(argc, argv, "f:vh")) != -1)
    {
        switch (option)
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
            print_usage(argv[0]);
            return 1;
        }
    }

    /*
      Initializare si citire configuratie.
    */
    config_init(&cfg);

    if (config_read_file(&cfg, "config/app.cfg") == CONFIG_FALSE)
    {
        fprintf(stderr,
                "Eroare config: %s:%d - %s\n",
                config_error_file(&cfg),
                config_error_line(&cfg),
                config_error_text(&cfg));
        config_destroy(&cfg);
        return 1;
    }

    /*
      Citire nume proiect din config.
    */
    if (config_lookup_string(&cfg, "project_name", &project_name) == CONFIG_FALSE)
    {
        fprintf(stderr, "Nu exista project_name in config.\n");
        config_destroy(&cfg);
        return 1;
    }

    /*
      Citire optiune pentru afisarea diagnosticelor.
    */
    if (config_lookup_bool(&cfg, "show_diagnostics", &show_diagnostics_from_config) == CONFIG_FALSE)
    {
        fprintf(stderr, "Nu exista show_diagnostics in config.\n");
        config_destroy(&cfg);
        return 1;
    }

    /*
      Citire lista de argumente pentru libclang.
    */
    clang_args_setting = config_lookup(&cfg, "clang_args");
    if (clang_args_setting == NULL)
    {
        fprintf(stderr, "Nu exista clang_args in config.\n");
        config_destroy(&cfg);
        return 1;
    }

    /*
      Fisierul sursa se ia prima data din -f.
      Daca -f lipseste, se foloseste variabila de mediu SOURCE_FILE.
    */
    const char *source_file = source_file_cli;
    if (source_file == NULL)
    {
        source_file = getenv("SOURCE_FILE");
    }

    if (source_file == NULL)
    {
        fprintf(stderr, "Nu a fost specificat fisierul sursa. Foloseste -f sau SOURCE_FILE.\n");
        config_destroy(&cfg);
        return 1;
    }

    /*
      -v are prioritate peste valoarea din config.
    */
    int show_diagnostics = (force_verbose != 0) ? 1 : show_diagnostics_from_config;

    /*
      Pregatire argumente pentru clang.
    */
    int arg_count = config_setting_length(clang_args_setting);
    const char *clang_args[MAX_CLANG_ARGS];

    if (arg_count < 0 || arg_count > MAX_CLANG_ARGS)
    {
        fprintf(stderr, "Numar invalid de argumente clang.\n");
        config_destroy(&cfg);
        return 1;
    }

    for (int i = 0; i < arg_count; i++)
    {
        const char *arg = config_setting_get_string_elem(clang_args_setting, i);
        if (arg == NULL)
        {
            fprintf(stderr, "Argument clang invalid in config.\n");
            config_destroy(&cfg);
            return 1;
        }

        clang_args[i] = arg;
    }

    /*
      Creare index libclang si parsare fisier.
    */
    CXIndex index = clang_createIndex(0, 0);

    CXTranslationUnit tu = clang_parseTranslationUnit(
        index,
        source_file,
        clang_args,
        arg_count,
        NULL,
        0,
        CXTranslationUnit_DetailedPreprocessingRecord);

    if (tu == NULL)
    {
        fprintf(stderr, "Libclang nu a putut parsa fisierul: %s\n", source_file);
        clang_disposeIndex(index);
        config_destroy(&cfg);
        return 1;
    }

    /*
      Parcurgere AST si colectare statistici.
    */
    AnalysisStats stats = {0, 0, 0, 0, 0};
    CXCursor root_cursor = clang_getTranslationUnitCursor(tu);
    (void)clang_visitChildren(root_cursor, ast_visitor, &stats);

    /*
      Colectare diagnostice.
    */
    unsigned diagnostic_count = clang_getNumDiagnostics(tu);

    /*
      Afisare rezultat.
    */
    printf("=== Analysis Result ===\n");
    printf("Project: %s\n", project_name);
    printf("Source file: %s\n", source_file);
    printf("Diagnostics count: %u\n", diagnostic_count);
    printf("Functions count: %u\n", stats.functions);
    printf("Variables count: %u\n", stats.variables);
    printf("If statements count: %u\n", stats.if_statements);
    printf("For loops count: %u\n", stats.for_loops);
    printf("While loops count: %u\n", stats.while_loops);

    if (show_diagnostics != 0)
    {
        printf("\n=== Diagnostics ===\n");

        if (diagnostic_count == 0U)
        {
            printf("Nu exista diagnostice.\n");
        }
        else
        {
            for (unsigned i = 0; i < diagnostic_count; i++)
            {
                CXDiagnostic diagnostic = clang_getDiagnostic(tu, i);
                CXString diag_text = clang_formatDiagnostic(
                    diagnostic,
                    clang_defaultDiagnosticDisplayOptions());

                printf("%s\n", clang_getCString(diag_text));

                clang_disposeString(diag_text);
                clang_disposeDiagnostic(diagnostic);
            }
        }
    }

    printf("\nParse completed.\n");

    /*
      Eliberare resurse.
    */
    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);
    config_destroy(&cfg);

    return 0;
}