#define __STDC_FORMAT_MACROS

#include <kdbproposal.h>
#include <kdbprivate.h>
#include "kdbtypes.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

/**
 * @brief validate array syntax
 *
 * @param key an element of an array
 *
 * @retval -1 if no array element/syntax error/no key
 * @retval 0 if start
 * @retval 1 if array element
 */
int elektraArrayValidateName(const Key *key)
{
	if (!key)
	{
		return -1;
	}

	const char *current = keyBaseName(key);

	if (!current)
	{
		return -1;
	}

	if (!strcmp(current, "#"))
	{
		return 0;
	}

	if (*current == '#')
	{
		current++;
		int underscores = 0;
		int digits = 0;

		for (; *current == '_'; current++)
		{
			underscores++;
		}

		for (; isdigit (*current); current++)
		{
			digits++;
		}

		if (underscores != digits -1) return -1;
		if (underscores + digits > ELEKTRA_MAX_ARRAY_SIZE-2)
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}

	return 1;
}

int elektraReadArrayNumber(const char *baseName, int64_t *oldIndex)
{

	int errnosave = errno;
	errno = 0;
	if (sscanf(baseName, "%"PRId64, oldIndex) != 1)
	{
		errno = errnosave;
		return -1;
	}

	if (errno != 0) // any error
	{
		errno = errnosave;
		return -1;
	}

	if (*oldIndex < 0) // underflow
	{
		return -1;
	}

	if (*oldIndex >= INT64_MAX) // overflow
	{
		return -1;
	}
	return 0;
}

/**
 * @internal
 *
 * @brief Writes a elektra array name
 *
 * @param newName the buffer to write to (size must be
 *       #ELEKTRA_MAX_ARRAY_SIZE or more)
 * @param newIndex the index of the array to write
 *
 * @retval 0 on success
 * @retval -1 on error
 */
int elektraWriteArrayNumber(char *newName, int64_t newIndex)
{
	// now we fill out newName
	size_t index = 0; // index of newName
	newName[index++] = '#';
	int64_t i = newIndex/10;

	while (i>0)
	{
		newName[index++] = '_'; // index n-1 of decimals
		i/=10;
	}
	if (snprintf (&newName[index], ELEKTRA_MAX_ARRAY_SIZE,
				"%"PRId64, newIndex)  < 0)
	{
		return -1;
	}

	return 0;
}

/**
 * @internal
 *
 * @brief Increment the name of the key by one
 *
 * Alphabetical order will remain
 *
 * e.g. user/abc/\#9 will be changed to
 *      user/abc/\#_10
 *
 * For the start:
 *      user/abc/\#
 * will be changed to
 *      user/abc/\#0
 *
 * @param key which base name will be incremented
 *
 * @retval -1 on error (e.g. too large array, not validated array)
 * @retval 0 on success
 */
int elektraArrayIncName(Key *key)
{
	const char * baseName = keyBaseName(key);

	int arrayElement = elektraArrayValidateName(key);
	if (arrayElement == -1)
	{
		return -1;
	}

	++baseName; // jump over #
	while(*baseName == '_') // jump over all _
	{
		++baseName;
	}

	int64_t oldIndex  = 0;
	if (!arrayElement)
	{
		// we have a start element
		oldIndex = -1;
	}
	else
	{
		if (elektraReadArrayNumber(baseName, &oldIndex) == -1)
		{
			return -1;
		}
	}

	int64_t newIndex = oldIndex+1; // we increment by one

	char newName[ELEKTRA_MAX_ARRAY_SIZE];

	elektraWriteArrayNumber(newName, newIndex);
	keySetBaseName(key, newName);

	return 0;
}

static int arrayFilter(const Key *key, void *argument)
{
	const Key *arrayParent = (const Key *) argument;

	if (!arrayParent) return 0;

	return keyIsDirectBelow(arrayParent, key) && elektraArrayValidateName(key);
}


/**
 *
 * Return all the array keys below the given arrayparent
 * The arrayparent itself is not returned.
 * For example, if user/config/# is an array,
 * user/config is the array parent.
 * Only the direct array keys will be returned. This means
 * that for eympale user/config/#1/key will not be included,
 * but only user/config/#1
 *
 * @param arrayParent the parent of the array to be returned
 * @param keys the keyset containing the array keys.
 *
 * @return a keyset containing the arraykeys (if any)
 * @retval NULL on NULL pointers
 */
KeySet *elektraArrayGet(const Key *arrayParent, KeySet *keys)
{
	if (!arrayParent) return 0;

	if (!keys) return 0;

	KeySet *arrayKeys = ksNew(ksGetSize(keys), KS_END);
	elektraKsFilterArgument(arrayKeys, keys, &arrayFilter, (void *)arrayParent);
	return arrayKeys;
}

/**
 *
 * Return the next key in the given array.
 * The function will automatically allocate memory
 * for a new key and name it accordingly. The
 * supplied keyset must contain only valid array keys.
 *
 * @param arraykeys the array where the new key will belong to
 *
 * @return the new array key on success
 * @retval NULL if the passed array is empty
 * @retval NULL on NULL pointers or if an error occurs
 */
Key *elektraArrayGetNextKey(KeySet *arrayKeys)
{
	if (!arrayKeys) return 0;

	Key *last = ksPop(arrayKeys);

	if (!last) return 0;

	ksAppendKey(arrayKeys, last);
	Key *newKey = keyDup(last);
	int ret = elektraArrayIncName(newKey);

	if (ret == -1)
	{
		keyDel(newKey);
		return 0;
	}

	return newKey;
}
