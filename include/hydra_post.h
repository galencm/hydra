/*  =========================================================================
    hydra_post - work with a single Hydra post

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of zbroker, the ZeroMQ broker project.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#ifndef __HYDRA_POST_H_INCLUDED__
#define __HYDRA_POST_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  @warning THE FOLLOWING @INTERFACE BLOCK IS AUTO-GENERATED BY ZPROJECT
//  @warning Please edit the model at "api/hydra_post.xml" to make changes.
//  @interface
//  This API is a draft, and may change without notice.
#ifdef HYDRA_BUILD_DRAFT_API
//  *** Draft method, for development use, may change without warning ***
//  Create a new post
HYDRA_EXPORT hydra_post_t *
    hydra_post_new (const char *subject);

//  *** Draft method, for development use, may change without warning ***
//  Destroy the post
HYDRA_EXPORT void
    hydra_post_destroy (hydra_post_t **self_p);

//  *** Draft method, for development use, may change without warning ***
//  Recalculate the post ID based on subject, timestamp, parent id, MIME
//  type, and content digest, and return post ID to caller.
HYDRA_EXPORT const char *
    hydra_post_ident (hydra_post_t *self);

//  *** Draft method, for development use, may change without warning ***
//  Return the post subject, if set
HYDRA_EXPORT const char *
    hydra_post_subject (hydra_post_t *self);

//  *** Draft method, for development use, may change without warning ***
//  Return the post timestamp
HYDRA_EXPORT const char *
    hydra_post_timestamp (hydra_post_t *self);

//  *** Draft method, for development use, may change without warning ***
//  Return the post parent id, or empty string if not set
HYDRA_EXPORT const char *
    hydra_post_parent_id (hydra_post_t *self);

//  *** Draft method, for development use, may change without warning ***
//  Return the post MIME type, if set
HYDRA_EXPORT const char *
    hydra_post_mime_type (hydra_post_t *self);

//  *** Draft method, for development use, may change without warning ***
//  Return the post content digest
HYDRA_EXPORT const char *
    hydra_post_digest (hydra_post_t *self);

//  *** Draft method, for development use, may change without warning ***
//  Return the post content location
HYDRA_EXPORT const char *
    hydra_post_location (hydra_post_t *self);

//  *** Draft method, for development use, may change without warning ***
//  Return the post content size
HYDRA_EXPORT size_t
    hydra_post_content_size (hydra_post_t *self);

//  *** Draft method, for development use, may change without warning ***
//  Return the post content as a string. Returns NULL if the MIME type is
//  not "text/plain". The caller must destroy the string when finished with it.
//  Caller owns return value and must destroy it when done.
HYDRA_EXPORT char *
    hydra_post_content (hydra_post_t *self);

//  *** Draft method, for development use, may change without warning ***
//  Set the post parent id, which must be a valid post ID
HYDRA_EXPORT void
    hydra_post_set_parent_id (hydra_post_t *self, const char *parent_id);

//  *** Draft method, for development use, may change without warning ***
//  Set the post MIME type
HYDRA_EXPORT void
    hydra_post_set_mime_type (hydra_post_t *self, const char *mime_type);

//  *** Draft method, for development use, may change without warning ***
//  Set the post content to a text string. Recalculates the post digest from
//  from the new content value. Sets the MIME type to text/plain.
HYDRA_EXPORT void
    hydra_post_set_content (hydra_post_t *self, const char *content);

//  *** Draft method, for development use, may change without warning ***
//  Set the post content to a chunk of data. Recalculates the post digest
//  from the chunk contents. Takes ownership of the chunk. The data is not
//  stored on disk until you call hydra_post_save.
HYDRA_EXPORT void
    hydra_post_set_data (hydra_post_t *self, const void *data, size_t size);

//  *** Draft method, for development use, may change without warning ***
//  Set the post content to point to a specified file. The file must exist.
//  Recalculates the post digest from the file contents. Returns 0 if OK, -1
//  if the file was unreadable.
HYDRA_EXPORT int
    hydra_post_set_file (hydra_post_t *self, const char *location);

//  *** Draft method, for development use, may change without warning ***
//  Save the post to disk under the specified filename. Returns 0 if OK, -1
//  if the file could not be created. Posts are always stored in the "posts"
//  subdirectory of the current working directory. Note: for internal use
//  only.
HYDRA_EXPORT int
    hydra_post_save (hydra_post_t *self, const char *filename);

//  *** Draft method, for development use, may change without warning ***
//  Load post from the specified filename. Posts are always read from the
//  "posts" subdirectory of the current working directory. Returns a new post
//  instance if the file could be loaded, else returns null.
//  Caller owns return value and must destroy it when done.
HYDRA_EXPORT hydra_post_t *
    hydra_post_load (const char *filename);

//  *** Draft method, for development use, may change without warning ***
//  Fetch a chunk of content for the post. The caller specifies the size and
//  offset of the chunk. A size of 0 means all content, which will fail if
//  there is insufficient memory available. The caller must destroy the chunk
//  when finished with it.
//  Caller owns return value and must destroy it when done.
HYDRA_EXPORT zchunk_t *
    hydra_post_fetch (hydra_post_t *self, size_t size, size_t offset);

//  *** Draft method, for development use, may change without warning ***
//  Encode a post metadata to a hydra_proto message
HYDRA_EXPORT void
    hydra_post_encode (hydra_post_t *self, hydra_proto_t *proto);

//  *** Draft method, for development use, may change without warning ***
//  Create a new post from a hydra_proto HEADER-OK message.
//  Caller owns return value and must destroy it when done.
HYDRA_EXPORT hydra_post_t *
    hydra_post_decode (hydra_proto_t *proto);

//  *** Draft method, for development use, may change without warning ***
//  Duplicate a post instance. Does not create a new post on disk; this
//  provides a second instance of the same post item.
//  Caller owns return value and must destroy it when done.
HYDRA_EXPORT hydra_post_t *
    hydra_post_dup (hydra_post_t *self);

//  *** Draft method, for development use, may change without warning ***
//  Print the post file to stdout
HYDRA_EXPORT void
    hydra_post_print (hydra_post_t *self);

//  *** Draft method, for development use, may change without warning ***
//  Self test of this class
HYDRA_EXPORT void
    hydra_post_test (bool verbose);

#endif // HYDRA_BUILD_DRAFT_API
//  @end

#ifdef __cplusplus
}
#endif

#endif
