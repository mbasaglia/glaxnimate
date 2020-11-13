Authors: Mattia Basaglia
disable_toc: 1
no_nav: 1

# Glaxnimate

<style>
.container
{
    text-align: center;
}

[role="main"] > ul
{
    display: flex;
    list-style: none;
    justify-content: center;
    padding: 0;
    margin: 1.5em 0;
    flex-flow: row wrap;
}

[role="main"] > ul li
{
    margin: 1ex;
}

[role="main"] > ul li a
{
    background: #008cba;
    color: #fff;
    padding: 1ex;
    border-radius: 5px;
    white-space: nowrap;
}

[role="main"] > ul li a:hover,
[role="main"] > ul li a:focus
{
    background: #00526e;
    text-decoration: none;
}
[role="main"] > ul li a:focus
{
    outline: thin dotted #008cba;
    outline-offset: 5px;
}

.cards {
    display: flex;
    justify-content: center;
    flex-flow: row wrap;
    align-items: stretch;
}

.card {
    display: flex;
    flex-flow: column;
    border: 1px solid #008cba;
    border-radius: 5px;
    background: #dff7ff;
    align-items: center;
    margin: .5em;
    padding: 1em;
/*     flex-grow: 1; */
/*     flex-basis: 0; */
    color: black;
    width: 300px;
}

.card img {
    width: 128px;
}

.card heading {
    font-weight: bold;
}
</style>


<img src="/img/logo.svg" width="128" />

Glaxnimate is a simple and fast vector graphics animation program.


## Download

* [:fa-download: Stable Version](download.md#stable-releases)
* [:fa-flask: Experimental Version](download.md#development-snapshots)
* [:fa-wrench: Source Code](contributing/read_me.md)

<a href="manual/"><img src="/img/screenshots/main_window.png" style="max-width: 100vw;"/></a>

## Features

<div class="cards">
    <a href="manual/ui/settings/" class="card">
        <img src="/img/ui/icons/preferences-desktop-theme-global.svg" />
        <heading>Customizable Interface</heading>
        <span>Dark and Light UI themes</span>
        <span>Icon themes</span>
        <span>Dockable views</span>
    </a>
    <a href="manual/" class="card">
        <img src="/img/ui/icons/draw-bezier-curves.svg" />
        <heading>Smooth Animations</heading>
        <span>Vector graphics and tweening animations</span>
    </a>
    <a href="download/" class="card">
        <img src="/img/ui/icons/computer.svg" />
        <heading>Cross Platform</heading>
        <span>Download for GNU/Linux, Windows, and Mac</span>
    </a>
    <a href="manual/formats/" class="card">
        <img src="/img/ui/icons/internet-web-browser.svg" />
        <heading>Animations for the web</heading>
        <span>Lottie animations</span>
        <span>Animated GIF and WebP</span>
        <span>Animated SVG</span>
    </a>
    <a href="contributing/scripting/" class="card">
        <img src="/img/ui/icons/preferences-plugin.svg" />
        <heading>Easily Extensible</heading>
        <span>You can use Python to manipulate animations and creating plugins.</span>
    </a>
</div>

## Learn More

* [:fa-book: User Manual](manual/index.md)
* [:fa-terminal: Scripting Guide](contributing/scripting/index.md)

## Get Involved

* [Donate](donate.md)
* [Contribute](contributing/index.md)
* [Report Issues](https://gitlab.com/mattbas/glaxnimate/-/issues)
* [Chat](https://t.me/Glaxnimate)
