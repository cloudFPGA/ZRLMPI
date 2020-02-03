// from: https://eli.thegreenplace.net/2011/03/07/from-c-to-ast-and-back-to-c-with-pycparser

const Entry* HashFind(const Hash* hash, const char* key)
{
    unsigned int index = hash_func(key, hash->table_size);
    Node* temp = hash->heads[index];

    while (temp != NULL)
    {
        if (!strcmp(key, temp->entry->key))
            return temp->entry;

        temp = temp->next;
    }

    return NULL;
}