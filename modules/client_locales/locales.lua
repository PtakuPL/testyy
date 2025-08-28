dofile 'neededtranslations'

-- private variables
local defaultLocaleName = 'en'
local installedLocales
local currentLocale
local localesWindow

-- send current locale to server (extended opcode)
local function sendLocale(localeName)
  local protocolGame = g_game.getProtocolGame()
  if protocolGame then
    protocolGame:sendExtendedOpcode(ExtendedIds.Locale, localeName)
    return true
  end
  return false
end

-- UI: create language picker window
local function createWindow()
  localesWindow = g_ui.displayUI('locales')
  local localesPanel = localesWindow:getChildById('localesPanel')
  local layout = localesPanel:getLayout()
  local spacing = layout:getCellSpacing()
  local size = layout:getCellSize()

  local count = 0
  for name, locale in pairs(installedLocales) do
    local widget = g_ui.createWidget('LocalesButton', localesPanel)
    widget:setImageSource('/images/flags/' .. name)
    widget:setText(locale.languageName)
    widget.onClick = function()
      selectFirstLocale(name)
    end
    count = count + 1
  end

  count = math.max(1, math.min(count, 3))
  localesPanel:setWidth(size.width * count + spacing * (count - 1))

  addEvent(function()
    addEvent(function()
      localesWindow:raise()
      localesWindow:focus()
    end)
  end)
end

function selectFirstLocale(name)
  if localesWindow then
    localesWindow:destroy()
    localesWindow = nil
  end
  if setLocale(name) then
    g_modules.reloadModules()
  end
end

-- hooks
local function onGameStart()
  if currentLocale then
    sendLocale(currentLocale.name)
  end
end

local function onExtendedLocales(protocol, opcode, buffer)
  local locale = installedLocales[buffer]
  if locale and setLocale(locale.name) then
    g_modules.reloadModules()
  end
end

-- helper to open the locales window from UI buttons/commands
function openLanguagePicker()
  if not localesWindow or not localesWindow:isVisible() then
    createWindow()
  else
    localesWindow:raise()
    localesWindow:focus()
  end
end

-- public functions
function init()
  installedLocales = {}
  installLocales('/locales')

  local savedLocale = g_settings.get('locale', 'false')
  if savedLocale ~= 'false' then
    setLocale(savedLocale)
  else
    setLocale(defaultLocaleName)
  end

  -- show picker on first run (or always, if you prefer: change to always connect onRun)
  if g_app.hasUpdater() then
    connect(g_app, { onUpdateFinished = createWindow })
  else
    -- show once on first run when no saved locale
    if savedLocale == 'false' then
      connect(g_app, { onRun = createWindow })
    end
  end

  ProtocolGame.registerExtendedOpcode(ExtendedIds.Locale, onExtendedLocales)
  connect(g_game, { onGameStart = onGameStart })

  -- export helper for other modules (e.g., topmenu button)
  modules = modules or {}
  modules.client_locales = modules.client_locales or {}
  modules.client_locales.openLanguagePicker = openLanguagePicker
  modules.client_locales.createWindow = createWindow
end

function terminate()
  installedLocales = nil
  currentLocale = nil

  ProtocolGame.unregisterExtendedOpcode(ExtendedIds.Locale)
  if g_app.hasUpdater() then
    disconnect(g_app, { onUpdateFinished = createWindow })
  else
    disconnect(g_app, { onRun = createWindow })
  end
  disconnect(g_game, { onGameStart = onGameStart })
end

function generateNewTranslationTable(localename)
  local locale = installedLocales[localename]
  for _i, k in pairs(neededTranslations) do
    local trans = locale.translation[k]
    k = k:gsub('\n', '\\n'):gsub('\t', '\\t'):gsub('\"', '\\\"')
    if trans then
      trans = trans:gsub('\n', '\\n'):gsub('\t', '\\t'):gsub('\"', '\\\"')
    end
    if not trans then
      print('    ["' .. k .. '"] = false,')
    else
      print('    ["' .. k .. '"] = "' .. trans .. '",')
    end
  end
end

function installLocale(locale)
  if not locale or not locale.name then
    error('Unable to install locale.')
  end

  if _G.allowedLocales and not _G.allowedLocales[locale.name] then
    return
  end

  if locale.name ~= defaultLocaleName then
    local updatesNamesMissing = {}
    for _, k in pairs(neededTranslations) do
      if locale.translation[k] == nil then
        updatesNamesMissing[#updatesNamesMissing + 1] = k
      end
    end
    if #updatesNamesMissing > 0 then
      pdebug("Locale '" .. locale.name .. "' is missing " .. #updatesNamesMissing .. " translations.")
      for _, name in pairs(updatesNamesMissing) do
        pdebug('["' .. name .. '"] = "",')
      end
    end
  end

  local installedLocale = installedLocales[locale.name]
  if installedLocale then
    for word, translation in pairs(locale.translation) do
      installedLocale.translation[word] = translation
    end
  else
    installedLocales[locale.name] = locale
  end
end

function installLocales(directory)
  dofiles(directory)
end

function setLocale(name)
  local locale = installedLocales[name]
  if locale == currentLocale then
    g_settings.set('locale', name)
    return
  end
  if not locale then
    pwarning('Locale ' .. name .. ' does not exist.')
    return false
  end
  if currentLocale then
    sendLocale(locale.name)
  end
  currentLocale = locale
  g_settings.set('locale', name)
  if onLocaleChanged then
    onLocaleChanged(name)
  end
  return true
end

function getInstalledLocales()
  return installedLocales
end

function getCurrentLocale()
  return currentLocale
end

-- global function used to translate texts
function _G.tr(text, ...)
  if currentLocale then
    if tonumber(text) and currentLocale.formatNumbers then
      local number = tostring(text):split('.')
      local out = ''
      local reverseNumber = number[1]:reverse()
      for i = 1, #reverseNumber do
        out = out .. reverseNumber:sub(i, i)
        if i % 3 == 0 and i ~= #number then
          out = out .. currentLocale.thousandsSeperator
        end
      end
      if number[2] then
        out = number[2] .. currentLocale.decimalSeperator .. out
      end
      return out:reverse()
    elseif tostring(text) then
      local translation = currentLocale.translation[text]
      if not translation then
        if translation == nil and currentLocale.name ~= defaultLocaleName then
          pdebug('Unable to translate: "' .. text .. '"')
        end
        translation = text
      end
      return string.format(translation, ...)
    end
  end
  return text
end
