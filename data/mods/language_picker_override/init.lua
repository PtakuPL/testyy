local langTopButton
local CONFIRM_FLAG = 'locale_confirmed_v1'

local function openLocalesWindow()
  if modules and modules.client_locales and modules.client_locales.createWindow then
    modules.client_locales.createWindow()
  end
end

function init()
  -- Add top-right menu button (near sound)
  if modules and modules.client_topmenu and modules.client_topmenu.addRightButton then
    local iconPath = '/images/topbuttons/options' -- change if you have a language/globe icon
    langTopButton = modules.client_topmenu.addRightButton('langTopButton', tr('Language'), iconPath, function()
      openLocalesWindow()
    end)
    if langTopButton and langTopButton.setTooltip then
      langTopButton:setTooltip(tr('Language'))
    end
  end

  -- Show language picker once after installing this mod
  if not g_settings.get(CONFIRM_FLAG, false) then
    addEvent(function()
      openLocalesWindow()
      g_settings.set(CONFIRM_FLAG, true)
    end)
  end
end

function terminate()
  if langTopButton and not langTopButton:isDestroyed() then
    langTopButton:destroy()
    langTopButton = nil
  end
end
