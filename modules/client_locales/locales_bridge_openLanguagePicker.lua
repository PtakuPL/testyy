-- === client_locales bridge: expose openLanguagePicker() for topmenu ===
modules = modules or {}
modules.client_locales = modules.client_locales or {}

if not modules.client_locales.openLanguagePicker then
  function modules.client_locales.openLanguagePicker()
    -- Try direct exported symbol
    if modules.client_locales.createWindow then
      return modules.client_locales.createWindow()
    end
    -- Try global function from this module
    if type(_G.createWindow) == 'function' then
      return _G.createWindow()
    end
    -- Fallback: schedule in case module not fully initialized yet
    if scheduleEvent then
      scheduleEvent(function()
        if modules.client_locales.createWindow then
          modules.client_locales.createWindow()
        elseif type(_G.createWindow) == 'function' then
          _G.createWindow()
        end
      end, 50)
    end
  end
end
-- === end bridge ===
