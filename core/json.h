#ifndef JSON_H
#define JSON_H

#include <b6/json.h>

#include "lib/io.h"
#include "lib/log.h"

extern struct b6_json *get_json(void);

extern void put_json(struct b6_json*);

struct json_istream {
	struct b6_json_istream up;
	struct istream *istream;
};

extern void setup_json_istream(struct json_istream *self,
			       struct istream *istream);

struct json_ostream {
	struct b6_json_ostream up;
	struct ostream *ostream;
};

extern void initialize_json_ostream(struct json_ostream *self,
				    struct ostream *ostream);

extern void finalize_json_ostream(struct json_ostream *self);

#endif /* JSON_H */
