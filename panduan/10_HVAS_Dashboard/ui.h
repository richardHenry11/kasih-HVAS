#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Call once after lv_init() and your LCD/touch driver are initialized. */
void ui_init(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_H */
