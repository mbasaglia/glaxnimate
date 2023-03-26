// SPDX-FileCopyrightText: 2021 Mattia Basaglia <dev@dragon.best>
// SPDX-License-Identifier: GPL-3.0-or-later

// TODO convert to JNI calls

package org.mattbas.glaxnimate.jnimessenger;

import java.util.ArrayList;

import android.content.ContentResolver;
import android.content.Intent;
import android.net.Uri;

//import android.app.Activity;

public class JniMessenger // extends Activity
{
    private static final String CREATE_STICKER_PACK_ACTION = "org.telegram.messenger.CREATE_STICKER_PACK";
    private static final String CREATE_STICKER_PACK_EMOJIS_EXTRA = "STICKER_EMOJIS";
    private static final String CREATE_STICKER_PACK_IMPORTER_EXTRA = "IMPORTER";

    private String generator;
    private ArrayList<Uri> stickers;
    private ArrayList<String> emojis;

    public Intent import_stickers()
    {
        Intent intent = new Intent(CREATE_STICKER_PACK_ACTION);
        intent.putExtra(Intent.EXTRA_STREAM, stickers);
        intent.putExtra(CREATE_STICKER_PACK_IMPORTER_EXTRA, generator);
        intent.putExtra(CREATE_STICKER_PACK_EMOJIS_EXTRA, emojis);
        intent.setType("image/*");

        return intent;
    }

    public JniMessenger(String generator)
    {
        this.generator = generator;
        stickers = new ArrayList<Uri>();
        emojis = new ArrayList<String>();
    }

    public void add_sticker(String file, String emoji)
    {
        stickers.add(Uri.parse("file://" + file));
        emojis.add(emoji);
    }
}
