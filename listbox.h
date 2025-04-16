int lb_reset(struct field *f);
int lb_insert(struct field *f, const char *update_str, int index);
int lb_count(struct field *f);
const char *lb_string_at(struct field *f, int index);
void lb_remove(struct field *f, int index);
void lb_dump(struct field *f);

//the listbox control function
int lb_fn(struct field *f, int method, void *ptr);
