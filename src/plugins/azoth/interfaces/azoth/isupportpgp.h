/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#pragma once

#include <QtGlobal>
#include <QtCrypto>

namespace LeechCraft
{
namespace Azoth
{
	/** @brief Interface for accounts supporting PGP encryption.
	 *
	 * This interface should be implemented by accounts which support
	 * PGP encryption, only if ENABLE_CRYPT compile-time option
	 * is enabled.
	 *
	 * @sa IAccount
	 */
	class ISupportPGP
	{
	public:
		virtual ~ISupportPGP () {}

		/** @brief Sets the private key for the account.
		 *
		 * @param[in] key The private key for the account.
		 */
		virtual void SetPrivateKey (const QCA::PGPKey& key) = 0;

		/** @brief Returns the private key for the account, if any.
		 *
		 * @return Private key for the account, or an empty key if no
		 * key has been set.
		 */
		virtual QCA::PGPKey GetPrivateKey () const = 0;

		/** @brief Sets the public key for the given entry.
		 *
		 * @param[in] entry The entry for which to set the public key.
		 * @param[in] pubKey The public key to set.
		 */
		virtual void SetEntryKey (QObject *entry, const QCA::PGPKey& pubKey) = 0;

		/** @brief Returns the public key for the given entry, if any.
		 *
		 * If there is no public key for the entry, an empty key should
		 * be returned.
		 *
		 * @param[in] entry The entry for which to return the public key.
		 * @return The public key for the given entry, if any.
		 */
		virtual QCA::PGPKey GetEntryKey (QObject *entry) const = 0;

		/** @brief Enables or disables encryption for the given entry.
		 *
		 * If the encryption has been enabled successfully, the
		 * encryptionStateChanged() should be emitted afterwards.
		 *
		 * @param[in] entry The entry for which to toggle the encryption.
		 * @param[in] enabled Whether encryption should be enabled.
		 */
		virtual void SetEncryptionEnabled (QObject *entry, bool enabled) = 0;
	protected:
		/** @brief Notifies whether signature has been verified for the
		 * given entry.
		 *
		 * This signal should be emitted whenever protocol receives a
		 * PGP signature from the given entry, no matter if that
		 * signature is valid or not. If the signature is valid,
		 * successful should be set to true, otherwise it should be
		 * false, but the signal still has to be emitted.
		 *
		 * @note This function is expected to be a signal.
		 *
		 * @param[out] entry The entry for which signature has been
		 * attempted to be verified.
		 * @param[out] successful Whether verification was successful.
		 */
		virtual void signatureVerified (QObject *entry, bool successful) = 0;

		/** @brief Notifies that encryption state has changed for the
		 * given entry.
		 *
		 * This signal should be emitted whenever encryption state
		 * changes for the given entry, even as the result of
		 * SetEncryptionEnabled() call.
		 *
		 * @param[out] entry The entry for which encryption state has
		 * changed.
		 * @param[out] enabled Whether encryption has been enabled or
		 * disabled.
		 */
		virtual void encryptionStateChanged (QObject *entry, bool enabled) = 0;
	};
}
}

Q_DECLARE_INTERFACE (LeechCraft::Azoth::ISupportPGP,
		"org.Deviant.LeechCraft.Azoth.ISupportPGP/1.0");
