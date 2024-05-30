#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define MAX_TP 128 
#define MAX_TLB 16 
#define MAX 2000
int *endereco;
int TLB[MAX_TLB]; 
int TP[MAX_TP];   
int mais_antigo[MAX_TP];
int num_pagesTP = 0; 
int pagina_removidaTP = 0;
int num_pagesTLB = 0;
int pagina_removidaTLB = 0;
int TLB_hits = 0;
int page_faults = 0;
int num_enderecos = 0;

char *decimal_to_binary(int number);
void split_binary(char *binary, char **first_half, char **second_half);
void read_and_convert_fifo(char *filename);
int binary_to_decimal(char *binary);
int check_TLB(int page);
int check_TP(int page);
int8_t get_signed_int_at_address(FILE *file, int endereco);
void read_and_convert_lru(char *filename);

int main(int argc, char *argv[])
{

    if (argc < 3) {
        printf("Usage: %s <filename> <fifo|lru>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[2], "fifo") == 0)
    {
        read_and_convert_fifo(argv[1]);
    }
    else if (strcmp(argv[2], "lru") == 0)
    {
        read_and_convert_lru(argv[1]);
    }
    else
    {
        printf("error.\n");
        return 1;
    }

    free(endereco);

    return 0;
}

char *decimal_to_binary(int number)
{
    char *binary = (char *)malloc(17 * sizeof(char));
    if (binary == NULL)
    {
        printf("Erro ao alocar memória.\n");
        exit(1);
    }

    binary[16] = '\0';

    for (int i = 15; i >= 0; i--)
    {
        binary[i] = (number & 1) ? '1' : '0';
        number >>= 1;
    }

    return binary;
}

void split_binary(char *binary, char **first_half, char **second_half)
{
    int length = strlen(binary);
    int mid = length / 2;
    
    *first_half = malloc((mid + 1) * sizeof(char));
    *second_half = malloc((length - mid + 1) * sizeof(char));

    if (*first_half == NULL || *second_half == NULL)
    {
        printf("Erro ao alocar memória.\n");
        exit(1);
    }

    strncpy(*first_half, binary, mid);
    (*first_half)[mid] = '\0';

    strncpy(*second_half, binary + mid, length - mid);
    (*second_half)[length - mid] = '\0';
}

int check_TLB(int page)
{
    for (int i = 0; i < num_pagesTLB; i++)
    {
        if (TLB[i] == page)
        {
            return 1;
        }
    }
    return 0;
}

int check_TP(int page)
{
    for (int i = 0; i < num_pagesTP; i++)
    {
        if (TP[i] == page)
        {
            return 1;
        }
    }
    return 0;
}

void read_and_convert_fifo(char *filename)
{
    FILE *file = fopen(filename, "r");
    FILE *fileback = fopen("BACKING_STORE.bin", "rb");
    FILE *output = fopen("correct.txt", "w");

    if (file == NULL)
    {
        printf("Não foi possível abrir o arquivo %s\n", filename);
        return;
    }

    if (fileback == NULL)
    {
        printf("Não foi possível abrir o arquivo BACKING_STORE.bin\n");
        fclose(file);
        return;
    }

    if (output == NULL)
    {
        printf("Não foi possível criar o arquivo correct.txt\n");
        fclose(file);
        fclose(fileback);
        return;
    }

    int n;
    int i = 0;
    int size = 1000;                       
    endereco = malloc(size * sizeof(int)); 
    int value;

    while (fscanf(file, "%d", &n) != EOF)
    {
        if (i == size)
        {                                                     
            size *= 2;                                        
            endereco = realloc(endereco, size * sizeof(int)); 
            if (endereco == NULL)
            { 
                exit(1);
            }
        }
        endereco[i] = n;
        value = get_signed_int_at_address(fileback, endereco[i]);
        i++;
        num_enderecos++;

        char *binary = decimal_to_binary(n);
        char *pagina = NULL;
        char *offset = NULL;
        split_binary(binary, &pagina, &offset);

        int pagina_decimal = binary_to_decimal(pagina);
        int offset_decimal = binary_to_decimal(offset);

        int TP_index = -1;  
        int TLB_index = -1; 

        if (check_TLB(pagina_decimal) && check_TP(pagina_decimal))
        {
            TLB_hits++;
            for (int j = 0; j < num_pagesTLB; j++)
            { 
                if (TLB[j] == pagina_decimal)
                {
                    TLB_index = j;
                    break;
                }
            }
            for (int j = 0; j < num_pagesTP; j++)
            { 
                if (TP[j] == pagina_decimal)
                {
                    TP_index = j;
                    break;
                }
            }
            fprintf(output, "Virtual address: %d TLB: %d Physical address: %d Value: %d\n", n, TLB_index, TP_index * 256 + offset_decimal, value);
        }
        else if (check_TP(pagina_decimal))
        {
            for (int j = 0; j < num_pagesTP; j++)
            { 
                if (TP[j] == pagina_decimal)
                {
                    TP_index = j;
                    break;
                }
            }
            if (num_pagesTLB < MAX_TLB)
            {
                TLB[num_pagesTLB] = pagina_decimal;
                TLB_index = num_pagesTLB; 
                num_pagesTLB++;
            }
            else
            {
                TLB[pagina_removidaTLB] = pagina_decimal;
                TLB_index = pagina_removidaTLB; 
                pagina_removidaTLB = (pagina_removidaTLB + 1) % MAX_TLB;
            }
            fprintf(output, "Virtual address: %d TLB: %d Physical address: %d Value: %d\n", n, TLB_index, TP_index * 256 + offset_decimal, value);
        }
        else
        {
            page_faults++;
            if (num_pagesTP < MAX_TP)
            {
                TP[num_pagesTP] = pagina_decimal;
                TP_index = num_pagesTP; 
                num_pagesTP++;
            }
            else
            {
                TP[pagina_removidaTP] = pagina_decimal;
                TP_index = pagina_removidaTP; 
                pagina_removidaTP = (pagina_removidaTP + 1) % MAX_TP;
            }

            if (num_pagesTLB < MAX_TLB)
            {
                TLB[num_pagesTLB] = pagina_decimal;
                TLB_index = num_pagesTLB; 
                num_pagesTLB++;
            }
            else
            {
                TLB[pagina_removidaTLB] = pagina_decimal;
                TLB_index = pagina_removidaTLB; 
                pagina_removidaTLB = (pagina_removidaTLB + 1) % MAX_TLB;
            }
            fprintf(output, "Virtual address: %d TLB: %d Physical address: %d Value: %d\n", n, TLB_index, TP_index * 256 + offset_decimal, value);
        }

        free(pagina);
        free(offset);
        free(binary);
    }
   fprintf(output, "Number of Translated Addresses = %d\n", num_enderecos);
    fprintf(output, "Page Faults = %d\n", page_faults);
    fprintf(output, "Page Fault Rate = %.3f\n", (float)page_faults / num_enderecos);
    fprintf(output, "TLB Hits = %d\n", TLB_hits);
    fprintf(output, "TLB Hit Rate = %.3f\n", (float)TLB_hits / num_enderecos);


    if (file != NULL) fclose(file);
    if (fileback != NULL) fclose(fileback);
    if (output != NULL) fclose(output);
}

void read_and_convert_lru(char *filename)
{
    FILE *file = fopen(filename, "r");
    FILE *fileback = fopen("BACKING_STORE.bin", "rb");
    FILE *output = fopen("correct.txt", "w");

    if (file == NULL)
    {
        printf("Não foi possível abrir o arquivo %s\n", filename);
        return;
    }

    if (fileback == NULL)
    {
        printf("Não foi possível abrir o arquivo BACKING_STORE.bin\n");
        fclose(file);
        return;
    }

    if (output == NULL)
    {
        printf("Não foi possível criar o arquivo correct.txt\n");
        fclose(file);
        fclose(fileback);
        return;
    }

    int n;
    int i = 0;
    int size = 1000;                       
    endereco = malloc(size * sizeof(int)); 
    int value;

    while (fscanf(file, "%d", &n) != EOF)
    {
        if (i == size)
        {                                                     
            size *= 2;                                        
            endereco = realloc(endereco, size * sizeof(int)); 
            if (endereco == NULL)
            { 
                exit(1);
            }
        }
        endereco[i] = n;
        value = get_signed_int_at_address(fileback, endereco[i]);
        i++;
        num_enderecos++;

        char *binary = decimal_to_binary(n);
        char *pagina = NULL;
        char *offset = NULL;
        split_binary(binary, &pagina, &offset);

        int pagina_decimal = binary_to_decimal(pagina);
        int offset_decimal = binary_to_decimal(offset);

        int TP_index = -1;  
        int TLB_index = -1; 

        if (check_TLB(pagina_decimal))
        {
            TLB_hits++;
            for (int j = 0; j < num_pagesTLB; j++)
            { 
                if (TLB[j] == pagina_decimal)
                {
                    TLB_index = j;
                    break;
                }
            }
            for (int j = 0; j < num_pagesTP; j++)
            { 
                if (TP[j] == pagina_decimal)
                {
                    TP_index = j;
                    break;
                }
            }
            mais_antigo[TP_index] = num_enderecos;
            fprintf(output, "Virtual address: %d TLB: %d Physical address: %d Value: %d\n", n, TLB_index, TP_index * 256 + offset_decimal, value);
        }
        else if (check_TP(pagina_decimal))
        {
            for (int j = 0; j < num_pagesTP; j++)
            { 
                if (TP[j] == pagina_decimal)
                {
                    TP_index = j;
                    break;
                }
            }
            mais_antigo[TP_index] = num_enderecos;
            if (num_pagesTLB < MAX_TLB)
            {
                TLB[num_pagesTLB] = pagina_decimal;
                TLB_index = num_pagesTLB; 
                num_pagesTLB++;
            }
            else
            {
                TLB[pagina_removidaTLB] = pagina_decimal;
                TLB_index = pagina_removidaTLB; 
                pagina_removidaTLB = (pagina_removidaTLB + 1) % MAX_TLB;
            }
            fprintf(output, "Virtual address: %d TLB: %d Physical address: %d Value: %d\n", n, TLB_index, TP_index * 256 + offset_decimal, value);
        }
        else
        {
            page_faults++;
            if (num_pagesTP < MAX_TP)
            {
                TP[num_pagesTP] = pagina_decimal;
                TP_index = num_pagesTP; 
                mais_antigo[num_pagesTP] = num_enderecos;
                num_pagesTP++;
            }
            else
            {
                int min = 0;
                for (int i = 1; i < num_pagesTP; i++)
                {
                    if (mais_antigo[i] < mais_antigo[min])
                        min = i;
                }
                TP[min] = pagina_decimal;
                TP_index = min;
                mais_antigo[min] = num_enderecos;
            }

            if (num_pagesTLB < MAX_TLB)
            {
                TLB[num_pagesTLB] = pagina_decimal;
                TLB_index = num_pagesTLB; 
                num_pagesTLB++;
            }
            else
            {
                TLB[pagina_removidaTLB] = pagina_decimal;
                TLB_index = pagina_removidaTLB; 
                pagina_removidaTLB = (pagina_removidaTLB + 1) % MAX_TLB;
            }
            fprintf(output, "Virtual address: %d TLB: %d Physical address: %d Value: %d\n", n, TLB_index, TP_index * 256 + offset_decimal, value);
        }

        free(pagina);
        free(offset);
        free(binary);
    }
    fprintf(output, "Number of Translated Addresses = %d\n", num_enderecos);
    fprintf(output, "Page Faults = %d\n", page_faults);
    fprintf(output, "Page Fault Rate = %.3f\n", (float)page_faults / num_enderecos);
    fprintf(output, "TLB Hits = %d\n", TLB_hits);
    fprintf(output, "TLB Hit Rate = %.3f\n", (float)TLB_hits / num_enderecos);

    if (file != NULL) fclose(file);
    if (fileback != NULL) fclose(fileback);
    if (output != NULL) fclose(output);
}

int binary_to_decimal(char *binary)
{
    int decimal = 0;
    int base = 1;
    int length = strlen(binary);

    for (int i = length - 1; i >= 0; i--)
    {
        if (binary[i] == '1')
        {
            decimal += base;
        }
        base *= 2;
    }

    return decimal;
}

int8_t get_signed_int_at_address(FILE *file, int endereco)
{
    int8_t value;
    fseek(file, endereco, SEEK_SET);
    fread(&value, sizeof(int8_t), 1, file);
    return value;
}
