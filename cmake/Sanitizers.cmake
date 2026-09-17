option(MT_ENABLE_ASAN "Enable AddressSanitizer for host C++ targets" OFF)
option(MT_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer where supported" OFF)

function(mt_apply_sanitizers tgt)
  if(MSVC)
    if(MT_ENABLE_ASAN)
      target_compile_options(${tgt} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/fsanitize=address>)
      target_link_options(${tgt} PRIVATE /fsanitize=address)
    endif()

    if(MT_ENABLE_UBSAN)
      message(WARNING "MT_ENABLE_UBSAN is not supported by MSVC and will be ignored for target ${tgt}")
    endif()
    return()
  endif()

  if(MT_ENABLE_ASAN)
    target_compile_options(
      ${tgt}
      PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-fsanitize=address>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-omit-frame-pointer>
    )
    target_link_options(${tgt} PRIVATE -fsanitize=address)
  endif()

  if(MT_ENABLE_UBSAN)
    target_compile_options(${tgt} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fsanitize=undefined>)
    target_link_options(${tgt} PRIVATE -fsanitize=undefined)
  endif()
endfunction()
