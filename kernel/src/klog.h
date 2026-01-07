#ifndef KLOG_H
#define KLOG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Níveis de Log */
#define KLOG_INFO  0
#define KLOG_WARN  1
#define KLOG_ERROR 2
#define KLOG_DEBUG 3

/* Inicializa o sistema de logs (reseta cursor, serial, etc) */
void klog_init(void);

/* * printk: Imprime formatado (estilo printf) direto no console.
 * Não adiciona prefixos ou quebras de linha automáticas.
 */
void printk(const char *fmt, ...);

/* * klog: Imprime mensagem formatada com prefixo de nível e quebra de linha.
 * Ex: klog(KLOG_INFO, "Sistema iniciado"); -> "[INFO] Sistema iniciado\n"
 */
void klog(int level, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* KLOG_H */

