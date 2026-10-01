/* SPDX-License-Identifier: GPL-2.0 */
/*
 * The vendor gen4m Wi-Fi driver ships its own small software crypto library
 * (wpa_supp/src/crypto).  Several of its symbol names now collide with the
 * mainline crypto/sha1.h and crypto/sha2.h APIs, which are pulled into the
 * same translation units via <linux/bpf.h>.
 *
 * Pull the mainline declarations first (so the renames below do not affect
 * them) and then rename the vendor symbols to a private namespace.
 */
#ifndef _MTK_WLAN_CRYPTO_COMPAT_H
#define _MTK_WLAN_CRYPTO_COMPAT_H

#include <crypto/sha1.h>
#include <crypto/sha2.h>

#define hmac_sha1	mtk_hmac_sha1
#define hmac_sha256	mtk_hmac_sha256
#define hmac_sha384	mtk_hmac_sha384
#define hmac_sha512	mtk_hmac_sha512
#define sha384_init	mtk_sha384_init
#define sha512_init	mtk_sha512_init

/* Vendor software AES collides with mainline lib/crypto/aes.c. */
#define aes_encrypt	mtk_aes_encrypt
#define aes_decrypt	mtk_aes_decrypt

#endif /* _MTK_WLAN_CRYPTO_COMPAT_H */
