/**
 * @file main.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2024-09-11
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SREC_LINE 80
#define SREC_HEADER "S0"
#define SREC_DATA "S1"
#define SREC_END "S9"

/**
 * @brief チェックサムの計算
 *
 * @param data 計算対象
 * @param len 計算対象の長さ
 * @return unsigned char チェックサム（8bit）
 */
unsigned char calc_checksum(const unsigned char *data, size_t len) {
    unsigned char checksum = 0;
    for (size_t i = 0; i < len; i++) {
        checksum += data[i];
    }
    return ~checksum;
}


/**
 * @brief アドレス長からレコードタイプの選択をする
 *
 * @param addr_size アドレスサイズ
 * @return const char* レコードタイプ
 */
int get_srec_type(int addr_size, char *str) {
    switch (addr_size) {
        case 2:
            str[0] = 'S';
            str[1] = '1';
            break;
        case 3:
            str[0] = 'S';
            str[1] = '2';
            break;
        case 4:
            str[0] = 'S';
            str[1] = '3';
            break;
        default:
            return -1;
    }

    return 0;
}


/**
 * @brief 1レコードをsrecファイルに書き出す
 *
 * @param output 書き出し先
 * @param type レコードタイプ
 * @param address アドレス
 * @param data データ
 * @param data_len データ長
 */
void write_srec(FILE *output, const char *type, unsigned int address, const unsigned char *data, size_t data_len) {
    unsigned char line[MAX_SREC_LINE];
    size_t i, idx = 0;
    unsigned char checksum;

    // レコードタイプ
    line[idx++] = type[0];
    line[idx++] = type[1];

    // データ長（アドレス+データ+チェックサム）
    size_t addr_len = type[1] == '1' ? 2 : type[1] == '2' ? 3 : 4;
    size_t record_len = addr_len + data_len + 1;  // チェックサム含む
    sprintf((char *)&line[idx], "%02X", (unsigned int)record_len);
    idx += 2;

    // アドレス部分
    for (i = 0; i < addr_len; i++) {
        sprintf((char *)&line[idx], "%02X", (unsigned int)((address >> ((addr_len - 1 - i) * 8)) & 0xFF));
        idx += 2;
    }

    // データ部分
    for (i = 0; i < data_len; i++) {
        sprintf((char *)&line[idx], "%02X", data[i]);
        idx += 2;
    }

    // チェックサム計算
    checksum = calc_checksum(line + 2, idx / 2 - 1);
    sprintf((char *)&line[idx], "%02X", checksum);
    idx += 2;

    // 改行
    line[idx++] = '\n';
    line[idx] = '\0';

    // 出力ファイルに書き込み
    fputs((char *)line, output);
}


/**
 * @brief main関数
 *
 * @param argc
 * @param argv
 * @return int
 *
 * @bug アドレス、テキストの入力がおかしい
 */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <binary_file>\n", argv[0]);
        return 1;
    }

    FILE *input_file = fopen(argv[1], "rb");
    if (input_file == NULL) {
        perror("Error opening binary file");
        return 1;
    }

    // 出力ファイルの作成
    FILE *output_file = fopen("output.srec", "w");
    if (output_file == NULL) {
        perror("Error creating S-record file");
        fclose(input_file);
        return 1;
    }

    // ファーストアドレスの入力
    unsigned int first_address;
    printf("Start address (hex): ");
    // scanf("%x", &first_address);

    // S0レコード用のテキスト入力
    char s0_text[32];
    unsigned int s0_len;
    printf("S0 record text (max 32 characters): ");
    // scanf("%s", s0_text);
    if (fgets(s0_text, 32, stdin) == NULL) {
        exit(EXIT_FAILURE);
    }
    s0_len = strlen(s0_text);
    if (s0_len > 0 && s0_text[s0_len - 1] == '\n') {
        // 改行だけ入力されたらS0レコードはなし
        s0_text[--s0_len] = '\0';
    }
    else if (s0_len > 0 && s0_text[s0_len - 1] == '\r') {
        // 改行だけ入力されたらS0レコードはなし
        s0_text[--s0_len] = '\0';
    }
    else {
        // S0レコードの作成
        write_srec(output_file, SREC_HEADER, 0x0000, (unsigned char *)s0_text, strlen(s0_text));
    }




    // Sレコードタイプの選択（S1, S2, S3）
    // int address_size;
    // printf("Address size (2 for S1, 3 for S2, 4 for S3): ");
    // scanf("%d", &address_size);

    // const char *srec_type = get_srec_type(address_size);
    // if (srec_type == NULL) {
    //     fprintf(stderr, "Invalid address size\n");
    //     fclose(input_file);
    //     fclose(output_file);
    //     return 1;
    // }

    // バイナリデータの変換
    unsigned char buffer[16];
    size_t bytes_read;
    unsigned int address = first_address;
    char srec_type[2];
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), input_file)) > 0) {
        if (address <= 0xFFFF) {
            get_srec_type(2, srec_type);
        } else if (address <= 0xFFFFFF) {
            get_srec_type(3, srec_type);
        } else {
            get_srec_type(4, srec_type);
        }
        write_srec(output_file, srec_type, address, buffer, bytes_read);
        address += bytes_read;
    }

    // S9エンドレコードの作成
    write_srec(output_file, SREC_END, first_address, NULL, 0);

    // ファイルクローズ
    fclose(input_file);
    fclose(output_file);

    printf("S-record file created successfully: output.srec\n");
    return 0;
}
