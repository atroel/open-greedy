#include "json.h"

#include <b6/json.h>
#include "lib/embedded.h"
#include "lib/io.h"
#include "lib/std.h"

static struct json {
	unsigned int refcount;
	struct b6_json json;
	struct b6_json_default_impl impl;
} json = { .refcount = 0, };

struct b6_json *get_json(void)
{
	if (json.refcount++)
		goto done;
	b6_json_default_impl_initialize(&json.impl, &b6_std_allocator);
	b6_json_initialize(&json.json, &json.impl.up, &b6_std_allocator);
done:
	return &json.json;
}

void put_json(struct b6_json *self)
{
	if (--json.refcount)
		return;
	b6_json_default_impl_finalize(&json.impl);
	b6_json_finalize(&json.json);
}

static long int istream_json_read(struct b6_json_istream *up, void *buf,
				  unsigned long int len)
{
	struct json_istream *self = b6_cast_of(up, struct json_istream, up);
	return read_istream(self->istream, buf, len);
}

void setup_json_istream(struct json_istream *self, struct istream *istream)
{
	static const struct b6_json_istream_ops ops = {
		.read = istream_json_read,
	};
	b6_json_setup_istream(&self->up, &ops);
	self->istream = istream;
}

static long int json_ostream_write(struct b6_json_ostream *up, const void *buf,
				   unsigned long int len)
{
	struct json_ostream *self = b6_cast_of(up, struct json_ostream, up);
	return write_ostream(self->ostream, buf, len);
}

static int json_ostream_flush(struct b6_json_ostream *up)
{
	struct json_ostream *self = b6_cast_of(up, struct json_ostream, up);
	return flush_ostream(self->ostream);
}

void initialize_json_ostream(struct json_ostream *self, struct ostream *ostream)
{
	static const struct b6_json_ostream_ops ops = {
		.write = json_ostream_write,
		.flush = json_ostream_flush,
	};
	b6_json_setup_ostream(&self->up, &ops);
	self->ostream = ostream;
}

void finalize_json_ostream(struct json_ostream *self)
{
	flush_ostream(self->ostream);
}
